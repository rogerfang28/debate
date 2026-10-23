#!/usr/bin/env python3
"""Delete chosen debates from a debates.sqlite3 (archiving them first), repair
the members of others, or restore archived debates.

Run with the server STOPPED, on a database that has been backed up (copy the
.sqlite3 file together with its -wal and -shm files: the data lives in the WAL).

    # dry run: says what would happen, writes nothing
    python3 prune_debates.py DB --delete 2,3,4 --repair 1,7 --archive old.sqlite3
    # do it
    python3 prune_debates.py DB --delete 2,3,4 --repair 1,7 --archive old.sqlite3 --apply
    # bring some back later (all of the archive when --ids is left out)
    python3 prune_debates.py DB --restore old.sqlite3 --ids 3 --apply
    # what is in an archive
    python3 prune_debates.py --list old.sqlite3

Delete: copies the debate and everything that belongs to it (claims, links,
moves, members, chats, briefs, summaries; whichever of those tables exist)
into the --archive file, then removes them. An archive is required: nothing
is deleted without a copy. The archive is an ordinary SQLite file with the
same tables, and can hold several runs.

Repair: a debate page shows only claims written by the debate's members
(BuildCollection::BuildForDebateAndUsers), and old debates often have no
member rows, so they render empty. This adds every claim author and link
author of the debate as a member. It never removes a member.

Restore: copies archived debates back under their original ids, by the
columns both files share (so an archive made before an upgrade restores into
the upgraded database). It refuses if any of those ids is taken. A restored
debate with no move log gets one from the server's backfill at its next start.

Each run is one transaction on DB: either all of it applies or none.
"""
import argparse
import os
import sqlite3
import sys

# Tables holding rows that belong to one debate, and the column naming it.
PER_DEBATE = [
    ("STATEMENTS", "DEBATE_ID"),
    ("LINKS", "DEBATE_ID"),
    ("MOVES", "DEBATE_ID"),
    ("DEBATE_MEMBERS", "DEBATE_ID"),
    ("CHATS", "DEBATE_ID"),
    ("SUMMARIES", "DEBATE_ID"),
    ("BRIEFS", "DEBATE_ID"),
]
ALL_TABLES = [("DEBATE", "ID")] + PER_DEBATE


def ids(text):
    return sorted({int(x) for x in text.split(",") if x.strip()}) if text else []


def tables_of(db, schema="main"):
    return {r[0] for r in db.execute(f"select name from {schema}.sqlite_master where type='table'")}


def columns_of(db, table, schema="main"):
    return [r[1] for r in db.execute(f"pragma {schema}.table_info({table})")]


def ensure_archive_tables(db):
    """Create in the attached archive any table it lacks, with the live columns.

    `create table ... as select ... where 0` copies the column names and types
    whatever the original CREATE statement looked like; an archive needs no
    keys or constraints of its own.
    """
    have = tables_of(db, "archive")
    live = tables_of(db)
    for table, _ in ALL_TABLES:
        if table in live and table not in have:
            db.execute(f"create table archive.{table} as select * from main.{table} where 0")


def copy_rows(db, src, dst, table, col, debate_id):
    """Copy one debate's rows of `table` from schema src to dst by shared columns."""
    if table not in tables_of(db, src) or table not in tables_of(db, dst):
        return 0
    shared = [c for c in columns_of(db, table, src) if c in set(columns_of(db, table, dst))]
    cols = ", ".join(shared)
    return db.execute(f"insert into {dst}.{table} ({cols}) select {cols} from {src}.{table} where {col} = ?",
                      (debate_id,)).rowcount


def list_archive(path):
    db = sqlite3.connect(f"file:{path}?mode=ro", uri=True)
    for did, topic in db.execute("select ID, TOPIC from DEBATE order by ID"):
        n = db.execute("select count(*) from STATEMENTS where DEBATE_ID=?", (did,)).fetchone()[0]
        print(f"#{did} '{topic}': {n} claims")
    db.close()


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("db", nargs="?")
    ap.add_argument("--delete", default="", help="comma-separated debate ids to archive and delete")
    ap.add_argument("--archive", help="archive file the deleted debates are copied into")
    ap.add_argument("--repair", default="", help="comma-separated debate ids whose members to repair")
    ap.add_argument("--restore", help="archive file to restore debates from")
    ap.add_argument("--ids", default="", help="with --restore: which debates (default: all in the archive)")
    ap.add_argument("--list", help="print the debates in an archive file and exit")
    ap.add_argument("--apply", action="store_true", help="write the changes (default: dry run)")
    args = ap.parse_args()

    if args.list:
        list_archive(args.list)
        return
    if not args.db:
        sys.exit("a database path is required")

    delete, repair = ids(args.delete), ids(args.repair)
    if set(delete) & set(repair):
        sys.exit(f"a debate cannot be both deleted and repaired: {sorted(set(delete) & set(repair))}")
    if delete and not args.archive:
        sys.exit("--delete needs --archive FILE: nothing is deleted without a copy")
    if args.restore and (delete or repair):
        sys.exit("restore on its own, not together with --delete or --repair")
    if args.restore and not os.path.exists(args.restore):
        sys.exit(f"no such archive: {args.restore}")

    db = sqlite3.connect(args.db)
    db.isolation_level = None  # transactions are explicit
    tables = tables_of(db)
    existing = {r[0]: r[1] for r in db.execute("select ID, TOPIC from DEBATE")}
    missing = [d for d in delete + repair if d not in existing]
    if missing:
        sys.exit(f"no such debate(s): {missing}")

    try:
        if args.restore:
            db.execute("attach database ? as archive", (args.restore,))
            archived = {r[0]: r[1] for r in db.execute("select ID, TOPIC from archive.DEBATE")}
            wanted = ids(args.ids) or sorted(archived)
            absent = [d for d in wanted if d not in archived]
            if absent:
                sys.exit(f"not in the archive: {absent}")
            db.execute("begin")
            for d in wanted:
                for table, col in ALL_TABLES:
                    if table == "DEBATE":
                        if d in existing:
                            raise SystemExit(f"debate id {d} is already in use; nothing restored")
                    elif table in tables and table in tables_of(db, "archive"):
                        idcol = "ID" if "ID" in columns_of(db, table) else None
                        if idcol:
                            clash = db.execute(
                                f"select count(*) from main.{table} where ID in (select ID from archive.{table} where {col} = ?)",
                                (d,)).fetchone()[0]
                            if clash:
                                raise SystemExit(f"{clash} {table} row id(s) of debate {d} are already in use; nothing restored")
                counts = []
                for table, col in ALL_TABLES:
                    n = copy_rows(db, "archive", "main", table, col, d)
                    if n and table != "DEBATE":
                        counts.append(f"{n} {table.lower()}")
                # A link archived before the upgrade has no RELATION_ID. The
                # server gives each such link its own relation (RELATION_ID =
                # ID, LinkDatabase.cc) when it adds the column, which has
                # already happened here, so restore does the same.
                if "LINKS" in tables and "RELATION_ID" in columns_of(db, "LINKS"):
                    db.execute("update main.LINKS set RELATION_ID = ID where DEBATE_ID = ? and RELATION_ID is null", (d,))
                print(f"restore #{d} '{archived[d]}': {', '.join(counts) or 'no rows besides the debate'}")
        else:
            if delete:
                # A dry run archives into memory, so it leaves no file behind.
                db.execute("attach database ? as archive", (args.archive if args.apply else ":memory:",))
            db.execute("begin")
            if delete:
                ensure_archive_tables(db)
                for d in delete:
                    if db.execute("select count(*) from archive.DEBATE where ID = ?", (d,)).fetchone()[0]:
                        raise SystemExit(f"debate {d} is already in the archive; use a fresh archive file")
            for d in delete:
                counts = []
                for table, col in ALL_TABLES:
                    if table not in tables:
                        continue
                    copy_rows(db, "main", "archive", table, col, d)
                    n = db.execute(f"delete from main.{table} where {col} = ?", (d,)).rowcount
                    if n and table != "DEBATE":
                        counts.append(f"{n} {table.lower()}")
                print(f"archive and delete #{d} '{existing[d]}': {', '.join(counts) or 'no rows besides the debate'}")

            for d in repair:
                have = {r[0] for r in db.execute("select USER_ID from DEBATE_MEMBERS where DEBATE_ID = ?", (d,))}
                authors = {r[0] for r in db.execute("select distinct CREATOR_ID from STATEMENTS where DEBATE_ID = ?", (d,))}
                if "LINKS" in tables:
                    authors |= {r[0] for r in db.execute("select distinct CREATOR_ID from LINKS where DEBATE_ID = ?", (d,))}
                add = sorted(a for a in authors - have if a and a > 0)
                for user_id in add:
                    db.execute("insert into DEBATE_MEMBERS (DEBATE_ID, USER_ID) values (?, ?)", (d, user_id))
                print(f"repair #{d} '{existing[d]}': members {sorted(have)} + added {add}")

        if args.apply:
            db.execute("commit")
            print("applied")
        else:
            db.execute("rollback")
            print("dry run: nothing written (add --apply to write)")
    except BaseException:
        if db.in_transaction:
            db.execute("rollback")
        raise
    finally:
        db.close()


if __name__ == "__main__":
    main()
