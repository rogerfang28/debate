#!/usr/bin/env python3
"""Delete chosen debates from a debates.sqlite3, and repair the members of others.

Run with the server STOPPED, on a database that has been backed up (copy the
.sqlite3 file together with its -wal and -shm files: the data lives in the WAL).

    python3 prune_debates.py DB --delete 2,3,4 --repair 1,7,18          # dry run
    python3 prune_debates.py DB --delete 2,3,4 --repair 1,7,18 --apply  # writes

Delete: removes the debate and everything that belongs to it (claims, links,
moves, members, chats, briefs, summaries), whichever of those tables exist, so
it works on a database from before the move log as well as after.

Repair: a debate page shows only claims written by the debate's members
(BuildCollection::BuildForDebateAndUsers), and old debates often have no
member rows, so they render empty. This adds every claim author and link
author of the debate as a member. It never removes a member.

Everything happens in one transaction: either all of it applies or none.
"""
import argparse
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


def ids(text):
    return sorted({int(x) for x in text.split(",") if x.strip()}) if text else []


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("db")
    ap.add_argument("--delete", default="", help="comma-separated debate ids to delete")
    ap.add_argument("--repair", default="", help="comma-separated debate ids whose members to repair")
    ap.add_argument("--apply", action="store_true", help="write the changes (default: dry run)")
    args = ap.parse_args()

    delete, repair = ids(args.delete), ids(args.repair)
    if set(delete) & set(repair):
        sys.exit(f"a debate cannot be both deleted and repaired: {sorted(set(delete) & set(repair))}")

    db = sqlite3.connect(args.db)
    tables = {r[0] for r in db.execute("select name from sqlite_master where type='table'")}
    existing = {r[0]: r[1] for r in db.execute("select ID, TOPIC from DEBATE")}
    missing = [d for d in delete + repair if d not in existing]
    if missing:
        sys.exit(f"no such debate(s): {missing}")

    try:
        db.execute("begin")
        for d in delete:
            counts = []
            for table, col in PER_DEBATE:
                if table in tables:
                    n = db.execute(f"delete from {table} where {col} = ?", (d,)).rowcount
                    if n:
                        counts.append(f"{n} {table.lower()}")
            db.execute("delete from DEBATE where ID = ?", (d,))
            print(f"delete #{d} '{existing[d]}': {', '.join(counts) or 'no rows besides the debate'}")

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
    except Exception:
        db.execute("rollback")
        raise
    finally:
        db.close()


if __name__ == "__main__":
    main()
