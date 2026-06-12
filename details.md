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
