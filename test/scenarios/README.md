# Writing a scenario (`.pbtxt`)

A scenario is a **data-driven test**: a text file describing what users do, and what must be true afterwards. No C++ — the runner (`test/tests/test_scenario_runner.cc`) executes it by forging real `DebateEvent`s and pushing them through `DebateModerator::handleRequest`, the same entry point the HTTP endpoint uses. So a scenario exercises dispatch, handlers, status propagation and the move log together, exactly as a user would.

This file is written to be **pasted to an AI as a spec**. Everything it needs is here.

---

## 1. Shape

```
name: "MyScenario"

steps {
  action { ... }            # optional
  expectations { ... }      # optional, repeatable
}
steps { ... }
```

Steps run **in order**. Within a step the action runs first, then its expectations. Both parts are optional, so you can:

- `action` only — do something, check nothing
- `expectations` only — assert current state
- both — do a thing and immediately check it

Because you can assert at any step, prefer **several small steps** over one big one at the end. A failure then points at the move that caused it.

---

## 2. Actions

```
action {
  username: "A"                 # required
  event_type: "ADD_CHILD_CLAIM" # required
  ...payload fields...
}
```

`username` is any string. Users must be created first with `CREATE_USER`.

`event_type` is either the literal `"CREATE_USER"` or the **exact name** of a value in the `EventType` enum (`protos/debate_event.proto`). An unknown name is a silent no-op, so copy names exactly.

### Supported actions

| `event_type` | Payload fields | Notes |
|---|---|---|
| `CREATE_USER` | — | Runner-level, not a real event. Do this first for every user |
| `CREATE_DEBATE` | `debate_topic` | Creates the debate **and its root claim** |
| `JOIN_DEBATE` | `debate_id` | Membership. Needed before a user's claims are visible |
| `ENTER_DEBATE` | `debate_id` | Puts the user *in* the debate, cursor at the root |
| `GO_TO_CLAIM` | `claim_id` | Moves the cursor. **Required before adding or challenging** |
| `OPEN_ADD_CHILD_CLAIM` | — | Optional; the add works without it |
| `ADD_CHILD_CLAIM` | `claim_text`, `extra_claim_texts` (repeated), `claim_description`, `basis` | Adds supporting claim(s) under the cursor |
| `SUBMIT_CHALLENGE_CLAIM` | `claim_text`, `extra_claim_texts` (repeated), `challenge_description`, `basis` | Challenges the claim at the cursor |
| `CONCEDE_CHALLENGE` | — | Target is resolved automatically (see below) |

Any other `EventType` name is accepted and dispatched, but the runner attaches **no payload**, so it only works for events that need none.

### The two rules people get wrong

**1. Position the cursor first.** `ADD_CHILD_CLAIM` and `SUBMIT_CHALLENGE_CLAIM` both act on wherever the user currently is. Always precede them with `GO_TO_CLAIM`.

**2. `CONCEDE_CHALLENGE` takes no argument.** The runner finds the challenge to concede: among `CHALLENGE` links in the debate whose *target claim was created by the conceding user*, it picks the one with the **highest challenging-claim id** — i.e. the most recent challenge against something that user said. If your scenario has several, be explicit in a comment about which one you mean.

### Multi-from relations

Several claims that **together** support one conclusion are ONE relation, not N:

```
action {
  username: "A"
  event_type: "ADD_CHILD_CLAIM"
  claim_text: "Rents rose 12% in controlled zones"
  extra_claim_texts: "Those zones saw no new construction"
  extra_claim_texts: "Supply shortage drives price"
  basis: "EVIDENTIAL"
}
```

That is different from three separate `ADD_CHILD_CLAIM` steps, which create three independent relations. Use `RELATION_COUNT` to pin which you meant.

**`SUBMIT_CHALLENGE_CLAIM` takes the same two fields, meaning the same things** — several sentences are ONE opposing relation ("these together refute that"), and `basis` says what kind of opposition it is.

`basis` ∈ `EVIDENTIAL` | `AUTHORITY` | `DEFINITIONAL` | `NORMATIVE`. Empty is allowed and means unspecified.

Setting it on a challenge is worth doing, because otherwise two quite different moves are stored identically:

```
basis: "EVIDENTIAL"    # "the evidence says otherwise" -- you are wrong about the world
basis: "DEFINITIONAL"  # "that is not what the word means" -- you are wrong about the word
```

---

## 3. Expectations

```
expectations { check_type: "..." ...fields... }
```

| `check_type` | Fields used | Asserts |
|---|---|---|
| `CLAIM_EXISTS` | `claim_id` | The claim is in the collection |
| `CLAIM_SENTENCE` | `claim_id`, `expected_claim_sentence` | Its text matches exactly |
| `USER_VIEW` | `claim_id`, `username`, `expected_status` | One user's view: `TRUE_CLAIM` \| `FALSE_CLAIM` \| `UNDETERMINED` |
| `COLLECTION_SIZE` | `expected_count` | Total claims **including the root** |
| `ENGAGEMENT_STATE` | `username`, `expected_action` | `ACTION_DEBATING` \| `ACTION_HOME` |
| `LINK_COUNT` | `link_type`, `expected_count` | Legacy view — see warning |
| `LINK_EXISTS` | `link_type`, `from_claim_id`, `to_claim_id` | Legacy view — see warning |
| `RELATION_COUNT` | `link_type`, `expected_count` | How many **relations** of that role |
| `RELATION_FROM_COUNT` | `to_claim_id`, `expected_count` | How many from-claims the relation targeting it has |
| `RELATION_FROM_EXISTS` | `from_claim_id`, `to_claim_id` | That claim is one of the sources |

`link_type` is `"PARENT_CHILD"` or `"CHALLENGE"`.

### Prefer `RELATION_*` over `LINK_*`

`LINK_COUNT` and `LINK_EXISTS` read the **legacy** single-edge view, which on a multi-from relation shows only its **first** source. A `LINK_EXISTS` for the second or third claim of a 3-from relation will **fail**, even though the relation is correct.

Use them only to pin what the legacy view degrades to. For anything about structure, use `RELATION_*`.

### Direction

`LINK_*` uses **stored** orientation: `from` = parent, `to` = child for support.
`RELATION_*` uses **semantic** orientation: `from` = the supporter, `to` = what is supported — the reverse for support relations. Challenges run challenger → challenged in both.

---

## 4. Claim ids

Ids are assigned **sequentially as claims are created**, starting at 1, shared across debates in the run.

- `CREATE_DEBATE` creates the root — that consumes an id
- `ADD_CHILD_CLAIM` with N texts consumes N ids
- `SUBMIT_CHALLENGE_CLAIM` consumes one

There is no way to name a claim, so you must count. **Put the expected id in a comment beside every action** — it makes the file reviewable and mistakes obvious:

```
# claim 4
steps { action { username: "A" event_type: "ADD_CHILD_CLAIM" claim_text: "..." } }
```

If ids come out wrong, run the scenario and read the `CLAIM_SENTENCE` failures — they tell you which id holds what.

---

## 5. Registering and running

Three steps, and **skipping the third is the usual first failure**:

1. Save as `test/scenarios/MyScenario.pbtxt`
2. Add a test in `test/tests/test_scenario_runner.cc`:

```cpp
TEST_F(ScenarioRunner, MyScenario) {
    TestScenario s = LoadScenarioFromFile("scenarios/MyScenario.pbtxt");
    ASSERT_GT(s.steps_size(), 0) << "Failed to load MyScenario.pbtxt";
    executeScenario(s, /*dump_json_per_step=*/true);
}
```

3. **Re-run CMake configure.** Scenario files are copied by a configure-time glob, so a new `.pbtxt` is not in the build directory until you reconfigure:

```bash
cmake -S test -B test/build && cmake --build test/build --target debate_simulation -j
```

Then:

```bash
cd test/build && ./debate_simulation.exe --gtest_filter=ScenarioRunner.MyScenario
```

Symptom of skipping step 3: `[LoadScenarioFromFile] FAILED to open`.

### Output

- `test/build/FINAL_<Name>.json` — final page state
- `test/build/FAIL_<Name>_step<N>.json` — written automatically on a failed expectation
- `test/build/json_output/<Name>/STEP_<Name>_step<N>.json` — per step, when `dump_json_per_step` is true

These are serialized `ui::Page` protos. The graph nodes and edges live under `stepViewMain → stepViewContentRow → stepViewRightColumn`.

---

## 6. What the engine cannot express

Do not write expectations for these — they will not pass, because the feature does not exist:

- **`OPPOSE` targeting a relation** ("your evidence is real but it doesn't prove that"). Only claims can be targeted
- **Statuses beyond three.** The engine produces `TRUE_CLAIM` / `FALSE_CLAIM` / `UNDETERMINED` only — there is no `CONTESTED`, `UNSUPPORTED`, `ACCEPTED`, `OPEN`, `STANDING`
- **Relation states** (`LIVE` / `DEAD` / `INERT`)
- **Moves** `REST`, `ACCEPT`, `REOPEN`, `TIMEOUT`
- **Lenses** — no per-reader view of the same data
- **Supporting/opposing counts**, confidence, weights or scores

If a spec calls for one of these, write the expressible part and state the omission in a comment. A scenario that quietly approximates a missing feature reports success the tool has not earned.

---

## 7. A complete example

```
name: "MultiFromSupport"

steps { action { username: "A" event_type: "CREATE_USER" } }

# claim 1 — the root
steps {
  action { username: "A" event_type: "CREATE_DEBATE" debate_topic: "Rent control raises rents" }
}
steps { action { username: "A" event_type: "JOIN_DEBATE"  debate_id: 1 } }
steps { action { username: "A" event_type: "ENTER_DEBATE" debate_id: 1 } }

# claims 2, 3, 4 — ONE relation with three from-claims
steps { action { username: "A" event_type: "GO_TO_CLAIM" claim_id: 1 } }
steps {
  action {
    username: "A"
    event_type: "ADD_CHILD_CLAIM"
    claim_text: "Rents rose 12% in controlled zones"
    extra_claim_texts: "Those zones saw no new construction"
    extra_claim_texts: "Supply shortage drives price"
    basis: "EVIDENTIAL"
  }
}

# One relation, three sources -- not three relations
steps {
  expectations { check_type: "COLLECTION_SIZE" expected_count: 4 }
  expectations { check_type: "RELATION_COUNT" link_type: "PARENT_CHILD" expected_count: 1 }
  expectations { check_type: "RELATION_FROM_COUNT" to_claim_id: 1 expected_count: 3 }
  expectations { check_type: "RELATION_FROM_EXISTS" from_claim_id: 2 to_claim_id: 1 }
  expectations { check_type: "RELATION_FROM_EXISTS" from_claim_id: 3 to_claim_id: 1 }
  expectations { check_type: "RELATION_FROM_EXISTS" from_claim_id: 4 to_claim_id: 1 }
  expectations { check_type: "USER_VIEW" claim_id: 2 username: "A" expected_status: "TRUE_CLAIM" }
}
```

Working files to copy from: `MultiFromRelation.pbtxt` (relation structure), `ConcedePropagationTest.pbtxt` (status propagation, with the reasoning written out), `FauciPaulHearing.pbtxt` (a real-world spec plus an explicit list of what it could not express).
