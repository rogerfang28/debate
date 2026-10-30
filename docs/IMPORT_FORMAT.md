# Importing a debate

Admins can add a whole sourced debate to a running site, without stopping it,
from **their own profile → Import debate**. Paste the text (or load a `.pbtxt`
/ `.txt` file with **Choose file**), press **Preview**, and if the preview has
no errors press **Publish**. Preview writes nothing. Publish builds the debate
step by step as you, exactly as if you had typed it, and opens it.

## Enabling admins

Set `DEBATE_ADMINS` in the server's environment to a comma-separated list of
usernames (case does not matter, spaces around names are ignored):

```
DEBATE_ADMINS=roger,phase2-tester
```

Unset or empty means nobody can import. The list is read from the environment,
so changing it needs a server restart. Nobody else sees the Import button, and
the server ignores import requests from anyone not on the list, and from
visitors always.

Locally: `$env:DEBATE_ADMINS="phase2-tester"; & backend\start-server.ps1 -Llm new`.

## Two formats

The format is detected automatically.

1. **A scenario file**: `debate_test::TestScenario` in protobuf text format,
   as written by `test/scenarios/generators/gen_*_sourced.py` (for example
   `test/scenarios/FauciCaseSourced.pbtxt`). It must create exactly one
   debate, act as one user (that user becomes you), and only add claims and
   challenges to its own debate: steps that delete, edit, concede, or name
   another debate or a claim it has not made are refused.

2. **An outline**, written by hand, described below.

## The outline

One claim per line. Blank lines and lines starting with `#` are ignored.

```
Root claim | Voice | https://source | "verbatim quote" | BASIS | note
  + A claim that supports the line above it | Voice | https://... | "quote"
    - A challenge to the line above it | Voice | https://... | "quote" | EVIDENTIAL | why
```

- **The first line is the root**, the debate's main claim. It has no `+` or `-`.
- **Every later line starts**, after its indentation, with `+` (it supports its
  parent) or `-` (it challenges its parent).
- **Its parent** is the nearest earlier line with *less* indentation. A tab
  counts as 4 spaces. A `+`/`-` line indented no deeper than the root has no
  parent and is an error.
- **Fields** are separated by `|` and trimmed; only the claim text is required.
  In order:

  | field | meaning |
  |---|---|
  | claim | the claim's text (cannot contain `\|`) |
  | voice | whose claim it is, e.g. `Dr. Smith`; blank = yours. A voice that is nobody yet becomes an account that never signs in |
  | source | the web page it was taken from (`http://` or `https://`) |
  | quote | the words on that page; surrounding `"` are removed |
  | basis | what kind of reason it is: `EVIDENTIAL`, `AUTHORITY`, `DEFINITIONAL`, `NORMATIVE` or `INFERENTIAL` (any case); blank = unspecified |
  | note | the claim's description |
  | image | a picture of the source, by its web address (`http://` or `https://`): a screenshot of a post, a chart. Shown under the quote in the claim panel |

  Leave a field empty to skip it: `Claim text | | | | NORMATIVE`.
  The root's basis and note are not used (the root has no parent, and a
  debate's root has no description); the preview warns about them.

### A complete example

```
# Should the town build the bypass? Sources opened 2026-10-29.
The town should build the eastern bypass | Town council | https://example.org/minutes | "The council recommends the eastern route."
  + It would take most lorries out of the high street | Highways report | https://example.org/traffic | "An estimated 70% of HGV movements would divert." | EVIDENTIAL
    - The estimate assumes lorries obey the new signage, which they did not on the A12 | Local paper | https://example.org/a12 | "Compliance fell to 40% within a year." | INFERENTIAL | The A12 scheme used the same signage.
  - It would cut through the water meadows, a protected habitat | Wildlife trust | https://example.org/meadows | "The route crosses a designated site." | NORMATIVE
    + The meadows are protected by law, not just by preference | Natural England | https://example.org/designation | "Designated in 1987." | AUTHORITY
```

This makes 5 claims: the root, 2 supports and 2 challenges, in 5 voices.

## Limits and checks

- At most 300 KB of text and 500 claims.
- The preview lists the format, the root claim, the number of claims
  (supports / challenges), every voice (marking the ones that will be created),
  and every error with its line number (outline) or the parser's message
  (scenario). **Publish** appears only when there are no errors, and publishes
  only the exact text that was previewed; edit it and you must preview again.
- A voice that is an existing account that signs in gets a warning: claims in
  that voice will be held by that person.
- If Publish fails part-way, the import view says at which step (and outline
  line) it stopped and names the partial debate it may have left, so it can
  be checked and deleted. A 500-claim import takes about half a minute.
- Every import is logged with the admin's username, the debate id and the
  claim count (`[Import] ...` in the server log).
