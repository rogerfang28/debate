DebateTool (placeholder name) — Structured Logical Debate Platform

## Table of Contents
- [The Core Problem: Misinformation](#the-core-problem-misinformation)
- [Demo (video may be provided later when tool is at MVP)](#demo-video-may-be-provided-later-when-tool-is-at-mvp)
- [Example Usage (WIP)](#example-usage-wip)
- [Technical Details](#technical-details)
- [Live Server](#live-server)
- [Author / Why I Built This](#author--why-i-built-this)

### The Core Problem: Misinformation
Misinformation is a prominent problem in the world today. For example, there are people advising against vaccination, which could lead to kids dying, yet current fact-checking tools fail by forcing blind trust in conflicting sources.
My tool addresses the spread of misinformation by mapping debates onto a 2D graph, creating a system of structural accountability that prevents users from dodging difficult facts.
By enforcing strict challenge rules that require debaters to directly answer counter-evidence or admit defeat, the platform cuts through manipulation to provide audiences with a clear, verified map of the truth.

For example, here is a case where my tool can thrive: an anti-vaccine influencer might claim that mercury in vaccines causes harm, only to be challenged with the fact that modern vaccines do not contain mercury. In real life, the influencer may try to dodge the issue by attacking the challenger, changing the subject, or ignoring the facts. In my tool, they cannot escape. They must either defeat the challenge with solid evidence or concede and withdraw the false claim entirely.
My tool guarantees a fair debate where every claim is tested for the truth. By breaking down each statement, the tool helps users see through complicated narratives to spot the vital flaws. 
An influencer might talk for hours to build a complex story. My tool cuts through the chaos to pinpoint the core truth for users: "Mercury might be harmful, but there is no mercury in vaccines." 
Over time, the tool becomes a hub of verified information. The next time that influencer gives a long speech, the audience can simply search the tool to find the exact rebuttal and the evidence proving them wrong.

## Demo (video may be provided later when tool is at MVP)
[View Debate Tool Demo](./demo.md)

### Creating A Debate
Home Page:

<img width="1903" height="937" alt="Screenshot 2026-03-01 193838" src="https://github.com/user-attachments/assets/2f6cf379-0be1-4607-9e58-bc0945df63a8" />

### Creating A Claim:
Menu:

<img width="1901" height="937" alt="Screenshot 2026-03-01 194205" src="https://github.com/user-attachments/assets/3737f9e7-62a1-42e3-a8e9-d2c9c4766d59" />

### Linking Claims:
Menu:

<img width="789" height="584" alt="Screenshot 2026-03-01 194351" src="https://github.com/user-attachments/assets/bb51cd41-eb97-4eea-8c9c-f0a8c5a30dca" />


### Challenging A Claim:
Menu:

<img width="1896" height="940" alt="image" src="https://github.com/user-attachments/assets/b7138776-0264-4ba4-8cc1-8c7ef9193187" />

New Challenge Debate Created:

<img width="1900" height="937" alt="image" src="https://github.com/user-attachments/assets/d67bc0d7-7801-43c3-87f0-7c1830694bda" />

### Example Debate Page

<img width="1897" height="902" alt="image" src="https://github.com/user-attachments/assets/9ce2f64e-4465-4716-b415-379a04c8ebf1" />

## Example Usage (WIP)
Example Topic: Should you vaccinate your children?

The tool will allow users to challenge illogical parts of a narrative
- RFK Jr makes the claim “vaccines cause autism” and doesn't provide proof
    - Challenge the claim to force opponent to provide proof, and they must respond to this challenge (very simple)
        - You are able to explain how this statement is not logical as there is no reason why it would be correct. If they provide evidence, it's even possible to break down and show exactly why it doesn't stand.
        - This is a simple case, which can be disproven quite easily by many tools including mine

Allows user to change opponent's argument into an equivalent one that is easier to challenge 
- Opponent claims “natural immunity is better than vaccines, so you shouldn’t vaccinate your children”
    - Challenge it and change it to a more scientific form so it is objective and can be proven true or false
        - “If this claim is true, then that means that children who take vaccines are more likely to die than children who don’t take it, correct?”
        - This can be easily proven by comparing data
            - Now what if they think the data is false? Then continue the debate on that point, because if it's false you lose, if it's true, you win. The debate has shrunk to a smaller form to prove one point which will logically conclude the argument.

Allows viewers to easily see where the weakness in a flawed narrative is
- Opponent claims “do not use vaccines because they contain mercury, which causes autism”
    - This claim I believe is false, so I must be able to prove some substatement is false
        - Substatements are like: Mercury causes autism. Vaccines have mercury.
        - I can disprove vaccines having mercury or at least enough to harm people.
    - Point out the main flaw of this claim to be “the thimerosal in vaccines has been researched to not cause side effects like autism or any other serious effect”
    - It's easier to see the proof of one smaller claim that breaks down the larger claim, than to address the larger claim immediately

## Technical Details
- Debates as graphs (somewhat like trees, with child claims helping to prove the parent claim)
- Challenges create sub-debates  
- Claims, proofs, and links  
    - Recursive claims, so that each claim has other claims that prove it underneath it
- New features in the future, ai incorporation, summaries of debates, more limitations to make sure people have to address points made

### Live Server
work in progress

## Author / Why I Built This
I built this project to solve a real world problem; in politics today, many figures make false claims, take actions to benefit themselves while pretending it benefits everyone, and manipulate a group of people to support them. The goal of this tool is to make it the easiest to see the TRUTH, and hardest for people to spread lies, as the holes in those lies can be pointed out and debunked.
