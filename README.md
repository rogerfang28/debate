DebateTool (placeholder name) — Structured Logical Debate Platform

## Table of Contents
- [What This Is](#what-this-is)
- [Example Usage](#example-usage)
- [The Problem](#the-problem)
- [DebateTool fixes this by](#debatetool-fixes-this-by)
- [Key Ideas](#key-ideas)
- [Demo](#demo)
- [Author / Why I Built This](#author--why-i-built-this)

## What This Is
DebateTool is a work in progress project, a platform for structured debates between users. It is designed with the purpose of representing debates and information, in a way that is easiest to digest and figure out the truth.

## Example Usage (WIP)
Example Topic: Should you vaccinate your children?

The tool will allow users to challenge illogical parts of a narrative
- RFK Jr makes the claim “vaccines cause autism” and doesn't provide proof
    - Challenge the claim to force opponent to provide proof, and they must respond to this challenge (very simple)
        - You are able to explain how this statement is not logical as there is no reason why it would be correct. If they provide evidence, it's even possible to break down and show exactly why it doesn't stand.
        - This is a simple case, which can be disproven quite easily by many tools including mine

Allows user to change opponents's argument into an equivalent one that is easier to challenge 
- Opponent claims “natural immunity is better than vaccines, so you shouldn’t vaccinate your children”
    - Challenge it and change it to a more scientific form so it is objective and can be proven true or false
        - “If this claim is true, then that means that children who take vaccines are more likely to die than children who don’t take it, correct?”
        - This can be easily proven by comparing data
            - Now what if they think the data is false? Then continue the debate on that point, because if it's false you lose, if it's true, you win. The debate has shrunk to a smaller form to prove one point which will logically conclude the argument.

Allows viewers to easily see where the weakness in a flawed narrative is
- Opponent claims “do not use vaccines because they contain mercury, which causes autism”
    - This claim I believe is false, so I must be able to prove some substatement is false
        - Substatements are like: Mercury causes austism. Vaccines have mercury.
        - I can disprove vaccines having mercury or at least enough to harm people.
    - Point out the main flaw of this claim to be “the thimerosal in vaccines has been researched to not cause side effects like autism or any other serious effect”
    - It's easier to see the proof of one smaller claim that breaks down the larger claim, than to address the larger claim immediately

## The Problem
It's hard to get accurate information with people making many different claims, and it's hard to figure out who is correct, when both sides seem set in stone.
Online debates today may:
- Reward loudness instead of accuracy  
- Mix emotional arguments with misinformation  
- Make it hard to understand why something is true or false: it's hard to figure out the other sides view in comparison to yours

## DebateTool fixes this by:
- Structuring arguments into clear logical trees, allowing for a better representation of both sides 
- Allowing users to challenge specific claims directly, pointing out weak points in the other's debate
    - This will allow people to see where they are wrong, or point out where their opponent is wrong, so that they don't waste time on points they both agree on.
- Making it easy to see where misinformation starts, so that it's possible to fix it and find the truth
    - **If a false claim is made, there HAS TO BE something wrong with it's proof. If you can point it out, challenge it, so that your opponent has to defend that part which you belive is wrong. If they can't defend it, they concede that statement, which would make their false claim invalid unless they fix it.**

## Key Ideas
- Debates as graphs (somewhat like trees, with child claims helping to prove the parent claim)
- Challenges create sub-debates  
- Claims, proofs, and links  
    - Recursive claims, so that each claim has other claims that prove it underneath it
- New features in the future, ai incorporation, summaries of debates, more limitations to make sure people have to address points made

## Demo (video may be provided later when tool is at MVP)
Images taken from testing of the tool:
<img width="1903" height="902" alt="image" src="https://github.com/user-attachments/assets/51b76535-c8eb-4ee3-92cd-84c457e1f2c0" />
<img width="1899" height="895" alt="image" src="https://github.com/user-attachments/assets/bd832b5b-7e78-4618-9bfd-8a0eb752dfe1" />


## Author / Why I Built This
I built this project to solve a real world problem; in politics today, many figures make false claims, take actions to benefit themselves while pretending it benefits everyone, and manipulates a group of people to support them. The goal of this tool is to make it the easiest to see the TRUTH, and hardest for people to spread lies, as the holes in those lies can be pointed out and debunked.
