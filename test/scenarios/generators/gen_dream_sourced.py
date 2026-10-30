# Emits test/scenarios/DreamCaseSourced.pbtxt and DreamCaseSourced.SOURCES.md:
# the allegations that the Minecraft YouTuber Dream cheated and faked content,
# entered by one reader ("guest") in the voices of the people who actually said
# each thing. Every claim carries the page it was taken from and the words on it.
#
# Research: four passes on 2026-10-29, one per strand (the 2020 speedrun case,
# the 2026 Manhunt hidden-cuts investigation, the 2026 Technoblade duel theory,
# and credibility and context). Every page was opened. Much of the 2026 material
# exists only as YouTube videos: excerpts marked (auto-captions) are YouTube's
# automatic captions, copied as captioned. A fifth pass (2026-10-30) filled
# gaps: Reddit comments by Dream (u/dreamistaken, flair "Dream") and repeater64
# are quoted from the original text, read through the Arctic Shift archive of
# Reddit because reddit.com itself was blocked; the links point at reddit.com.
import io
import os

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "DreamCaseSourced.pbtxt")
OUT_SOURCES = os.path.join(HERE, "..", "DreamCaseSourced.SOURCES.md")

MOD_REPORT = "https://mcspeedrun.com/dream.pdf"
MOD_REBUTTAL = "https://archive.org/details/rebuttal_202106"
PHOTO = "https://drive.google.com/file/d/1yfLURFdDhMfrvI2cFMdYM8f_M_IRoAlM/view"
PASTEBIN = "https://web.archive.org/web/20210530135206/https://pastebin.com/raw/7nfjLwi3"
KARL_2026 = "https://www.youtube.com/watch?v=6RGqs-Nyhq8"
KARL_OCT = "https://www.youtube.com/watch?v=Sd5SQ40sCoI"
DART = "https://www.youtube.com/watch?v=WsWNWzSwVUA"
SCRIPTLINE = "https://www.youtube.com/watch?v=lBauWJX3aAo"
ERDGEIST = "https://www.youtube.com/watch?v=fJFL-ZoxsOE"
GLESTIC_DUEL = "https://www.youtube.com/watch?v=OuwEMf8khRw"
DREAM_LOGS = "https://www.youtube.com/shorts/iSwIiTNVvWo"
MASTERWYATT = "https://www.youtube.com/watch?v=Q934KcLk9gM"
MANGO = "https://www.youtube.com/watch?v=2lkxXfMFu2Y"
ELIOTEX = "https://www.youtube.com/watch?v=VVFywSmvUL4"

def reddit(sub, thread, comment=""):
    return f"https://www.reddit.com/r/{sub}/comments/{thread}/" + (f"comment/{comment}/" if comment else "")

# Dream's two October 2026 Reddit comments, and repeater64's answers.
DREAM_R1 = reddit("DreamWasTaken2", "1x261yj", "pf0ho3x")      # 10 Oct, 08:59 UTC
DREAM_R1_AIM = reddit("DreamWasTaken2", "1x261yj", "pf2r52w")  # 10 Oct, 16:45 UTC
DREAM_R2 = reddit("DreamWasTaken2", "1x6ahzn", "pgvktp5")      # 15 Oct, 11:12 UTC
REP_R = reddit("DreamWasTaken2", "1x6ahzn", "pgw78sa")         # 15 Oct, 13:15 UTC
REP_R_AI = reddit("DreamWasTaken2", "1x6ahzn", "pgw7id0")      # 15 Oct, 13:16 UTC

# (key, kind, anchor_key, voice, text, basis, url, excerpt, description)
# kind: "root" | "support" (anchor = parent) | "challenge" (anchor = target)
# basis: one of backend/config/bases.pbtxt: EVIDENTIAL, AUTHORITY, DEFINITIONAL,
# NORMATIVE, INFERENTIAL.
#
# Shape: the root is Dream's own account, and every allegation is a challenge
# to it. Each claim answers its parent -- a reason for it, or a reply against
# it -- never a loose fact filed nearby. Each branch is a thread in date
# order: the accusation, Dream's answer, the reply to that, entered oldest
# first so the map and the Brief read as the argument actually went. Claim
# text is one short sentence with its reason; context, dates and caveats live
# in the description.
N = []
def S(key, parent, voice, text, basis, url, excerpt, desc=""):
    N.append((key, "support", parent, voice, text, basis, url, excerpt, desc))
def C(key, target, voice, text, basis, url, excerpt, desc=""):
    N.append((key, "challenge", target, voice, text, basis, url, excerpt, desc))

AC = " (Quote from YouTube's auto-captions.)"

# ---- Root: Dream's account -------------------------------------------------------
N.append(("root", "root", None, "Dream",
    "Dream never knowingly cheated: his one cheated speedrun was an accident, and his Manhunts, records and fights are real.",
    "", "https://www.youtube.com/watch?v=18jB0zQysgg",
    "I did unintentionally cheat on a speedrun that I officially submitted to the leaderboards in 2020 and the speedrun mods were completely in the right for taking it down",
    "Dream's position, pieced together from his own words: 'The Truth' (December 2023), where he called the speedrun cheat unintentional and said it had nothing to do with his Manhunt videos, and his 2026 denials on Reddit and YouTube, quoted in the branches below. "
    "Each branch is one allegation against it, told in order: the accusation, Dream's answer, and the replies." + AC))

# ---- 1. The 2020 speedrun, and was it an accident? (Dec 2020 - Oct 2026) --------
C("a1", "root", "Speedrun mods",
  "Dream's 2020 speedruns ran on a modified game: ender pearls dropped about three times as often as the game allows.",
  "EVIDENTIAL", MOD_REPORT,
  "The only sensible conclusion that can be drawn after this analysis is that Dream's game was modified in order to manipulate the pearl barter and rod drop rates.",
  "December 2020. The volunteer moderators of Minecraft speedrunning on speedrun.com, in a published report. They removed his Minecraft 1.16 run.")
S("a1_counts", "a1", "Speedrun mods",
  "On six streams, 42 of 262 trades gave ender pearls and 211 of 305 blazes gave rods, against normal rates of about 5% and 50%.",
  "EVIDENTIAL", MOD_REPORT,
  "The data collected show that 42 of the 262 piglin barters performed throughout these streams yielded ender pearls, and 211 of the 305 killed blazes dropped blaze rods.",
  "The raw counts, which nobody disputes. They were first gathered by the speedrunners MinecrAvenger and danhem9; MinecrAvenger was banned in 2022 for faking his own runs (PC Gamer), but the finding rests on the moderators' own analysis.")
S("a1_odds", "a1", "Speedrun mods",
  "Even after correcting in Dream's favour, luck like his would happen at most once in 7.5 trillion.",
  "EVIDENTIAL", MOD_REPORT,
  "This is about 1 in 7.5 trillion. ... This is a loose (i.e., almost certainly an overestimate) upper bound on the chance that anyone in the Minecraft speedrunning community would ever get luck comparable to Dream's",
  "The odds that any speedrunner ever gets this lucky, not the chance Dream is innocent. Before any correction, the pearls alone were 1 in 177 billion.")
C("a1_cherry", "a1_odds", "Dream",
  "The sample is cherry-picked: it starts when his luck looked odd and stops at his best time.",
  "INFERENTIAL", "https://web.archive.org/web/20210106221537/https://dotesports.com/streaming/news/dream-minecraft-speedrun-controversy-a-history-of-events",
  "Rather than taking a truly random sample, the sample starts when the luck seems off, and the sample ends as soon as I hit a personal best goal of 19 minutes",
  "Dream's first defence, December 2020, as quoted by Dot Esports.")
C("a1_cherry_r", "a1_cherry", "Speedrun mods",
  "The report already assumes the most unfavourable stopping point, so cherry-picking is accounted for.",
  "INFERENTIAL", MOD_REPORT,
  "To account for the stopping rule, we will correct for the worst possible (most biased) stopping rule.",
  "From the original report: it built in the correction Dream asked for.")
C("a1_photo", "a1_odds", "Photoexcitation",
  "The moderators misapplied their corrections: the odds are nearer 1 in 10 million.",
  "EVIDENTIAL", PHOTO,
  "I identify two major issues with the MST Report: it does not account for stopping bartering after a successful trade and it incorrectly applies some bias corrections.",
  "21 December 2020. An anonymous astrophysicist whose report Dream paid for. It also argued that counting all eleven of Dream's streams shows no significant evidence. Its own summary still called cheating, deliberate or not, an obvious possibility.")
C("a1_photo_r", "a1_photo", "Speedrun mods",
  "That figure is miscalculated, and even taken at face value it still means a 99.99999% chance he cheated.",
  "INFERENTIAL", MOD_REBUTTAL,
  "Even if Dream's response calculation were done correctly, the 1 in 10 million posterior probability would not be directly comparable to the 1 in 7.5 trillion figure and would still imply a 99.99999% chance of Dream cheating.",
  "The moderators' rebuttal, December 2020. On the eleven streams: Dream is only accused over the last six, so adding the normal ones just dilutes the data.")
S("a1_karl", "a1_photo_r", "Karl Jobst",
  "Simulating the game trillions of times never came close to Dream's luck, so odds in the millions are wrong.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=f8TlTaTHgzo",
  "this means that figures such as one in a million one in ten million or even one in a hundred million are demonstrably false",
  "31 December 2020." + AC)
C("a1_concede", "a1_photo", "Photoexcitation",
  "Even read as generously as possible, the drop rates were very likely modified, his own follow-up found.",
  "EVIDENTIAL", "https://archive.org/details/agreements-counterarguments-dream_202106",
  "Even in the best-possible interpretation, the probability that any speedrunner had any streak of luck as strong as Dream's is extremely low, providing significant evidence that the ender pearl and/or blaze rod drop probabilities were modified",
  "8 January 2021. Dream's own expert, after the rebuttal, in a follow-up partly paid for by Dream.")
S("a1_parker", "a1_odds", "Matt Parker",
  "As a neutral mathematician: the odds are so extreme these runs should not count as records.",
  "AUTHORITY", "https://www.youtube.com/watch?v=8Ko3TdPy0TU",
  "i'm not assigning guilt i'm just saying that definitively as a disinterested party i can confirm these speed runs done by dream should not be included in the list of world records",
  "February 2021, Stand-up Maths. He found the odds about ten billion times less likely than any officially recognised result in any game, and deliberately said nothing about intent." + AC)
# May 2021: Dream concedes the modified game, and the argument moves to whether
# he knew.
S("a1_admit", "a1", "Dream",
  "A disallowed mod was in his game on about six streams, put there by a developer without his knowledge.",
  "EVIDENTIAL", PASTEBIN,
  "After considering this, I ended up finding out that I HAD actually been using a disallowed modification during ~6 of my live streams on Twitch.",
  "May 2021, after five months of denial. He said the developer had copied the boosted drop rates from his challenge-video servers into a mod he used while streaming, and that the moderators were 'mostly right'. The moderators told Vice the run's validity was 'no longer under contention'.")
S("intent_karl21", "a1_admit", "Karl Jobst",
  "After interviews, chat logs and the developer's own account, Dream most likely didn't know.",
  "AUTHORITY", "https://www.youtube.com/watch?v=G3Yzk-3SZfs",
  "in my opinion it is definitely more likely that he really didn't know his drop rates and barter rates were modified",
  "June 2021. Karl reversed this view in 2026." + AC)
C("a1_admit_r", "a1_admit", "MrMangoHands",
  "The developer's story doesn't hold: the loading-speed mod his mod was said to copy didn't exist until December 2020, after the runs.",
  "EVIDENTIAL", MANGO,
  "I'm the one who legalized it for use in speedruns in 2021, not 2020. In fact, this mod didn't even exist until December of 2020 after Dreams runs.",
  "23 October 2026. A former leaderboard moderator. The mod is LazyDFU. He also says the explanation shifted from a 'chat mod' (October 2020) to an in-house recording mod (May 2021), and that right after the 19-minute run Dream switched game versions and swapped out his mods." + AC)
C("intent_karl26", "intent_karl21", "Karl Jobst",
  "The developer behind that account was Callahan, Dream's own seed-finder, and Karl now believes he was in on it.",
  "INFERENTIAL", KARL_OCT,
  "So, it finally dawned upon me, long overdue I must admit, that of course Callahan would be in on it, too.",
  "29 October 2026, the first time the developer was named publicly. His reasons: Dream's friends play along with staged Manhunts, and George and Sapnap watched the 1.15 record run live. Dream named Callahan as a seed-finder on Reddit in November 2025." + AC)

# ---- 2. A parkour macro (Feb - Mar 2026) ------------------------------------------
C("mcc", "root", "Luna",
  "Dream used a jump macro: in his 2020 Championship parkour he jumped at the last possible moment 225 times out of 226, too consistent for a human.",
  "EVIDENTIAL", KARL_2026,
  "tweet by Luna who claimed that Dream jumped from the last possible tick 225 out of 226 times. She also uploaded a video showing 225 tick perfect jumps in a row. The claim is that this is impossible.",
  "February 2026. Luna's post couldn't be opened; this is Karl Jobst's account of it. A 'last possible tick' jump leaves one game tick (50 milliseconds) to press jump. Sportskeeda covered the wave of accusations of 'perfect one-pixel jumps'." + AC)
C("mcc_dream", "mcc", "Dream",
  "Long runs of tick-perfect jumps are easy: he filmed himself landing 50 in a row with his feet on the desk.",
  "EVIDENTIAL", KARL_2026,
  "there's footage playing of me in the background with a hand cam hitting [music] 50 in a row with my feet both on the desk just to be silly.",
  "February 2026, a Short Dream posted in reply, as played in Karl Jobst's video." + AC)
S("mcc_dream_live", "mcc_dream", "Dream",
  "Live on a hand-cam stream he hit 273 one-tick jumps in his first two runs, falling once.",
  "EVIDENTIAL", reddit("DreamWasTaken2", "1rd8s5x", "o74jku3"),
  "In the first TWO runs, I hit 273 1 tick jumps, while falling forward one time.",
  "24 February 2026, on Reddit. He says that beats the Championship numbers called impossible, and that the missing spacebar sounds are his noise gate, set to filter out keyboard sounds; Karl Jobst later agreed with the noise-gate point.")
C("mcc_karl", "mcc", "Karl Jobst",
  "The Championship statistic fails: most counted jumps had visual cues, and at a top player's 92.7% success rate, 69 cue-less jumps in a row is only 1 in 186.",
  "EVIDENTIAL", KARL_2026,
  "The best performer was Lily, who achieved a success rate of 92.7%. With this kind of success rate, the odds of getting 69 non-visual one-tick jumps in a row is 1 in 186. Those odds aren't outrageous.",
  "March 2026. He also noted that Dream accidentally ran off an edge in that event, which an auto-jump should prevent. So the Championship numbers don't prove it for him; his evidence of a macro is from 2023, the next point." + AC)
S("mcc_2023", "mcc", "Karl Jobst",
  "The macro shows in Dream's 2023 parkour streams instead: whenever he snags on a pole he never jumps, and in 8 hours he never once jumps too late.",
  "EVIDENTIAL", KARL_2026,
  "in every single case that he gets stuck on a pole in this way, he doesn't jump.",
  "March 2026, from Dream's November 2023 streams. Also: 'I could not find a single instance where Dream accidentally jumped too late and ran off an edge. Yes, 8 hours in a row.' He calls this 'the clearest evidence' of a macro, and says a cheater caught twice makes 'accidental' hard to believe. Dream answered the Championship accusations at length on Reddit, but not these points about the 2023 streams." + AC)
C("mcc_pb", "mcc_dream", "Karl Jobst",
  "The hand-cam demo was billed as his first time on the course in four years, but his personal best there was set in November 2023.",
  "EVIDENTIAL", KARL_2026,
  "In his hand cam video Dream uploaded in the description, he said that he hadn't played the course in 4 years, but his personal best was achieved just over 2 years ago in November 2023.",
  "March 2026. The hand-cam video (February 2026) does say 'I haven't played this course in like 4 years'." + AC)
C("mcc_dream_r", "mcc_dream", "Dart Spark",
  "Landing jumps that have visual cues says nothing about the cue-less jumps under suspicion.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=S5d5FMmMo-U",
  "being able to do 100% of visual jumps does not translate to being able to do 100% of non-visual jumps.",
  "22 March 2026, 'Debunking Dream's Responses With Parkour Tick Mechanics'." + AC)

# ---- 3. The Technoblade duel (Feb - Oct 2026) -------------------------------------
C("c1", "root", "GlesticOfficial",
  "Dream used aim assist for years, including in his 2020 duel with Technoblade.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=ca-NuR3Syug",
  "Through the process, I realized that Dream had been using the same cheat for perhaps his entire career, or at least the wide majority of it. This includes his manhunts, Minecraft Championships, and worst of all– his $100,000 duel with Technoblade.",
  "February 2026. The duel was a best of ten hosted by MrBeast in August 2020; Technoblade won 6 to 4, and died in 2022. In his July follow-up Glestic admits the evidence 'is not conclusive'.")
S("c1_stat", "c1", "GlesticOfficial",
  "In the duel Dream landed a perfectly timed hit on all 29 weapon swaps; Technoblade managed 7.",
  "EVIDENTIAL", GLESTIC_DUEL,
  "Guess how many tick perfect swings Dream had out of 29 swaps. And the answer is every single one.",
  "July 2026. His own count from about six minutes of footage." + AC)
S("c1_logs", "c1", "GlesticOfficial",
  "Technoblade knew, and beat the cheat by stripping logs, which old cheat clients mistook for walls.",
  "INFERENTIAL", GLESTIC_DUEL,
  "Techno tries to play this off by blaming his ADHD, but to me it is obvious what he is really trying to do.",
  "July 2026. He calls this part 'pure speculation'. It went viral on X on 23 and 24 October." + AC)
C("c1_logs_tb", "c1_logs", "Technoblade",
  "He hit the ground by accident while trying to raise his shield.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=5MfhpJOkCKc",
  "i try to raise my shield but accidentally uh sheer the ground or whatever you call this mechanic all i know is that i clearly did not take my adhd medicine this morning",
  "Technoblade's own analysis video, August 2020, years before the theory. He never suggested cheating: 'we don't have actual beef it's all video games.'" + AC)
C("c1_logs_dream", "c1_logs", "Dream",
  "No cheat client has ever stopped aiming because of stripped logs.",
  "EVIDENTIAL", DREAM_LOGS,
  "It's not something that's ever been in a cheat client of any kind. It's never been a thing.",
  "25 October 2026. He also asked people to stop using his late friend this way." + AC)
C("c1_logs_dream_r", "c1_logs_dream", "GlesticOfficial",
  "One did, a client called Astolfo; there's no clip of it only because that client no longer exists.",
  "EVIDENTIAL", "https://www.youtube.com/post/Ugkxy0eY2SDwJsdgYs1o8VkLLhEXiLJj6v4z",
  "We had no clips of stripped logs disabling aimbot, because the original client that made this happen (Astolfo) no longer exists.",
  "About 25 October 2026. So the effect has never been shown working with the client he names.")
C("c1_astolfo", "c1_logs_dream_r", "endlesslyeight",
  "Astolfo only ran on version 1.8.9, which has no shields, and its decompiled code has no such feature.",
  "EVIDENTIAL", reddit("DreamWasTaken2", "1xffg6l"),
  "Astolfo client was only released for version 1.8.9",
  "25 October 2026, a Reddit post citing a decompiled release of the client. The duel was fought with shields, which arrived in version 1.9.")
C("c1_dart", "c1", "Dart Spark",
  "There's no convincing evidence Dream cheated in the duel.",
  "AUTHORITY", "https://www.youtube.com/post/UgkxV_GiYLsYkA2DhphCWizQxZ-BcqAGem9m",
  "I still see no convincing evidence that Dream cheated in the 100k duel and that is an opinion I have been publicly stating for months.",
  "About 25 October 2026. Dart Spark leads the Manhunt investigation, so this comes from a critic.")
C("c1_karl", "c1", "Karl Jobst",
  "Nothing he has seen shows Dream cheated in the duel.",
  "AUTHORITY", KARL_OCT,
  "I have seen no evidence that convinces me that Dream cheated in that duel.",
  "29 October 2026, also a long-time critic." + AC)

# ---- 4. Staged Manhunt worlds (Oct 2026) ------------------------------------------
C("e1", "root", "masterwyatt",
  "Dream's early Manhunts were staged: the worlds don't match their seeds, and a death was cut out.",
  "EVIDENTIAL", MASTERWYATT,
  "if you observe that there's a brick pattern that matches a seed uh in your video, screenshot, whatever, and the eyes don't match, that means it was modified without any doubt.",
  "6 October 2026, checked with a seed-finder. Manhunt is Dream's series where he speedruns while friends hunt him. Other mismatches: a ruined-portal chest without the Frost Walker boots Dream used, a desert temple's loot, and slimes outside slime chunks." + AC)
S("e1_pearl", "e1", "masterwyatt",
  "In the 3 Hunters Rematch, the last ender pearl Dream needed came from a chest that, on the real seed, doesn't hold one.",
  "EVIDENTIAL", MASTERWYATT,
  "the fact that in this one chest there's a singular pearl missing with every other piece of loot generating the same. Especially since this was also the last pearl Dream needed.",
  "Karl Jobst replays this in his 29 October video." + AC)
C("e1_pearl_dream", "e1_pearl", "Dream",
  "He wouldn't spawn a pearl into a chest when he could just kill one more Enderman.",
  "INFERENTIAL", MASTERWYATT,
  "I was killing Enderman for so so long. Why would I just spawn in a pearl in a chest instead of just kill one extra Enderman?",
  "A clip from an earlier Dream stream, played in masterwyatt's video (about 19:06)." + AC)
S("e1_soulsand", "e1", "masterwyatt",
  "On the real seed, a soul sand block in the lava would have blocked the boat clutch; in the video it's missing.",
  "EVIDENTIAL", MASTERWYATT,
  "A singular soul sand block that didn't exist in his video that would have blocked the clutch.",
  "He rules out look-alike seeds; a custom server jar is the only innocent explanation he leaves open. At the time he judged the clutch itself not provably staged; the hidden cut and the furnace came three days later (see the 'uncut' Manhunts)." + AC)
S("e1_pool", "e1", "Dream",
  "Dream himself had said a ravine water pool he once clutched in was probably placed by his seed-finders.",
  "EVIDENTIAL", reddit("DreamWasTaken", "1oqfc0q", "np835rd"),
  "We play on custom server jar's, mods, and many things that could potentially slightly tweak small things like world gen, however I think in this case it was probably actually added!",
  "16 November 2025, on Reddit, before the investigation. He named the seed-finders as Callahan and Ken, and said the early Manhunts had looser rules.")
S("e1_death", "e1", "masterwyatt",
  "In the second-ever Manhunt Dream died, and both his and George's videos cut the death out.",
  "EVIDENTIAL", MASTERWYATT,
  "you always drop exactly 100 experience points, which happens to match perfectly with Dream's experience level after the cut. This guarantees Dream died in this manhunt and both him and George cut it out of the video.",
  "He allows the death may have been unfair, as Dream had connection problems at the time." + AC)

# ---- 5. The "uncut" Manhunts (Oct 2026) -------------------------------------------
C("b1", "root", "Dart Spark",
  "Dream's three original \"uncut\" Manhunts hide 46 cuts, found by reading the game's clock from the hunger bar.",
  "EVIDENTIAL", DART,
  "New evidence was just discovered that suggests Dream cuts his supposedly uncut Manhunts in order to fake them. And not just once or twice, but 46 different times across the three original videos.",
  "9 October 2026, with help from Angle, repeater64, Fanda857 and masterwyatt. The speedrun investigators had already found splices in the Manhunts by May 2023 (Eliotex, 23 October 2026)." + AC)
# How the cuts were found, and the fight over whether the method works.
S("b1_method", "b1", "Dart Spark",
  "With no saturation, the hunger icons jolt in a pattern set by the game's tick counter, so the jolts reveal the exact game time.",
  "EVIDENTIAL", DART,
  "So, what is the seed that's used for the random hunger bar jolt pattern? Well, it turns out to be none other than the value of the tick counter.",
  "The tick counter rises 20 times a second while the game runs; a jump means missing footage. The tools were published with the video. The method itself was found by Crafterdark in early 2023." + AC)
S("b1_valid", "b1_method", "Dart Spark",
  "The method finds no cuts in a livestream and exactly the known cuts in a confirmed fake.",
  "EVIDENTIAL", DART,
  "What we're looking at here is a manhunt by Feinberg which was live streamed and obviously isn't going to have any hidden cuts in it. And we see exactly what we'd expect, a single straight unbroken line.",
  "His two control tests." + AC)
C("b1_setup", "b1_method", "Dream",
  "The trick doesn't apply cleanly to his custom server and mods.",
  "INFERENTIAL", DREAM_R1,
  "It's definitely a useful trick to find cuts in speedruns and stuff, but obviously doesn't apply cleanly to custom server jars, custom mods, etc.",
  "10 October 2026, on Reddit (r/DreamWasTaken2, as u/dreamistaken), the day after the video.")
C("b1_setup_r", "b1_setup", "Erdgeist After Dark",
  "An independent rebuild of the method matches Dart's data, and it reads Dream's newest Manhunt, on his current setup, as honestly edited.",
  "EVIDENTIAL", ERDGEIST,
  "I took 40 random readings from Dart's published data and my tool returned the same tick count for all 40. So this does two things. It validates that my tool works and it also validates the result of Dart Spark.",
  "22 October 2026. Reading Dream's new 6 Hunters Finale, he found every cut moves time forward, as honest editing would: the method works on Dream's current setup, and that video comes out clean." + AC)
C("b1_setup_gwim", "b1_setup_r", "TheGwimWeeper",
  "A clean scan proves little: a simple mod can pause the jolts, and Dart's tool then finds no cuts.",
  "EVIDENTIAL", reddit("DreamWasTaken2", "1xc9gy6"),
  "Made a simple mod to pause and resume the jolts with keybinds and checked it for cuts, the program used by Dart Spark reported no cuts with only lag/pause being present",
  "22 October 2026, with a demonstration video. It shows a clean result can be engineered; it doesn't explain the cuts that were found.")
C("b1_endrf", "b1_valid", "Endrf",
  "The method can mislead: with a screenshot-marker mod he built, one unbroken take showed jolt \"cuts\".",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=3dBafTfH-Rc",
  "This video so far has been one whole take, not a single cut.",
  "27 October 2026, a computer-science student; the mod's source is public. He also argues two of the 46 cuts are misreadings. He doesn't address the backward jumps, the 3.5-hour splice, the furnace or the hotbar." + AC)
# What the cuts hide.
S("b1_numbers", "b1", "Dart Spark",
  "One \"uncut\" video shows 2 hours 13 minutes of a game that ran over 4 hours 45 minutes.",
  "EVIDENTIAL", DART,
  "That brings our total up to 14 cuts. The video that was delivered to our screens was 2 hours and 13 minutes long. And yet the true time spent was upwards of 4 hours and 45 minutes.",
  "The 5 Hunters video. The other two: 21 cuts (1h46m shown of about 8h18m) and 11 cuts (3h of about 7h50m)." + AC)
S("b1_splice", "b1", "Dart Spark",
  "One cut jumps the clock by three and a half hours, so footage was spliced in from another session.",
  "EVIDENTIAL", DART,
  "he has an absolutely unbelievable uptime of 3 and 1/2 hours. No, this is not a bug in the scan. This is real.",
  "At that clock time, Dream was elsewhere in the same video." + AC)
S("b1_retry", "b1", "Dart Spark",
  "The cuts hide retries: the original series was faked by secretly cutting out failed attempts, while the newer series scans clean.",
  "INFERENTIAL", DART,
  "I believe the original Manhunt series was faked by doing multiple attempts and secretly cutting out the failed ones, while the new series is most likely real.",
  "His conclusion." + AC)
C("b1_retry_hour", "b1_retry", "Dream",
  "The save-state story fails: there's no cut for almost an hour after the boat clutch.",
  "INFERENTIAL", DREAM_R1,
  "He says that the short \"cuts\" where nothing is happening makes sense because they are to save progress, but then there is no \"cut\" for almost an hour after the boat clutch lmao",
  "10 October 2026, on Reddit.")
C("b1_prod", "b1_retry", "Dream",
  "Restarting is a disclosed rule, not a secret: an early death doesn't count, and the run starts over on a new seed.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=km1IlBAZJH0",
  "So these deaths at the beginning don't actually count as a run. The seed is lost and we move down the list of seeds to the next seed.",
  "Dream's own production video, January 2026, made before the investigation. It covers deaths right after the start; it doesn't address the hidden cuts found in the middle of runs, and its description says it leaves out the editing.")
# The boat clutch, the series' most famous moment.
S("b1_clutch", "b1", "repeater64",
  "A hidden cut sits about 15 seconds before the famous boat clutch, and Dream relaunched his game during it, so the clutch wasn't one attempt.",
  "EVIDENTIAL", DART,
  "there is a cut hidden even closer to the boat clutch, just around 15 seconds before it. Since an unknown amount of time [music] passed during this cut and Dream relaunched his game,",
  "repeater64, a plugin developer on the investigation team, speaking in Dart Spark's video. He withdrew one of his own 2022 claims about the clutch (a jump that turned out to be a bug)." + AC)
S("b1_furnace", "b1_clutch", "Dart Spark",
  "On the clutch tower a furnace smelts 2.44 ingots in 15.1 seconds when the game allows 1.51, so 9.3 seconds of footage are missing.",
  "EVIDENTIAL", DART,
  "There's a whole 9.3 seconds of footage missing from the video after he loaded his save state.",
  "The check was done by Leila on the investigation team. One ingot takes exactly 10 seconds; every other smelt in the video was never off by more than a frame or two, so it isn't lag. The game clock also drops from 7 to 6 minutes there, meaning Dream relaunched the game, mid-smelt, on the clutch tower." + AC)
C("b1_clutch_dream", "b1_clutch", "Dream",
  "He improvised the clutch in one take; the main video only cut out his thinking.",
  "EVIDENTIAL", reddit("DreamWasTaken", "mx3nv9", "gvm4hmv"),
  "I was originally planning to jump down to the crafting bench below so I knew it was there (you'll see in the uncut). just cut out all of the thinking basically.",
  "23 April 2021, on Reddit, answering the first boat-clutch doubts; he also explained it on a livestream. He added that the server's crafting reach was longer than in normal survival.")
C("b1_clutch_hotbar", "b1_clutch_dream", "repeater64",
  "Frame by frame, the boat appears in his bottom hotbar several frames before he clicks it in the inventory.",
  "EVIDENTIAL", DART,
  "if you step through Dream's boat clutch footage frame by frame, you'll notice something incredibly weird. The boat appears in his hot bar at the bottom of the screen several frames before he actually clicks it into his inventory",
  "The mismatch was first spotted in April 2021; repeater64 lays it out in Dart Spark's video, 9 October 2026." + AC)
C("b1_clutch_hotbar_dream", "b1_clutch_hotbar", "Dream",
  "The early boat in the bottom hotbar is just a visual glitch, maybe from a mod.",
  "EVIDENTIAL", reddit("DreamWasTaken2", "mxw8tw", "gvrwbo4"),
  "I assume it’s just minecraft being confused like it always is, could be a mod thing though.",
  "25 April 2021, when the mismatch was first raised.")
C("b1_clutch_hotbar_r", "b1_clutch_hotbar_dream", "repeater64",
  "It can't be a glitch: both hotbars draw the same item, so they can never disagree; the crafting footage was laid over a separate clip.",
  "EVIDENTIAL", DART,
  "render based on the exact same object, making it completely impossible for them to be out of sync, even for a single frame.",
  "9 October 2026, in Dart Spark's video, answering the glitch explanation. YouTube compression can make the two differ, but it delays the bottom one, the opposite of what's seen. A recreation by other YouTubers shows the same flaw." + AC)
# Dream's explanations, and the replies.
C("b1_mod", "b1", "Dream",
  "The glitches come from a mod that takes screenshots to mark moments for editing, not from cuts.",
  "EVIDENTIAL", DREAM_R1,
  "I have a mod that takes screenshots whenever I trigger it, in order to make the editing process easier by marking key moments or things that might need to get editing attention.",
  "10 October 2026, on Reddit. He says the screenshot clears chat, fog and tooltips, so it looks like a cut, and that saving it on the main thread caused the lag spikes.")
C("b1_mod_r", "b1_mod", "scriptline",
  "A screenshot mod can't make the item shimmer jump, because it runs on the computer's clock, not the game's.",
  "EVIDENTIAL", SCRIPTLINE,
  "The glint animation derives its time value from Java's system.nanoTime. While the value returned by this function will depend on your machine, there is absolutely no means or reason for this value to ever be influenced by a mod.",
  "11 October 2026. He built his own screenshot mod to test it; the shimmer didn't jump." + AC)
TAYLER_AI = "https://www.youtube.com/watch?v=YWG8YRjPJsk"
C("b1_mod2", "b1_mod_r", "Dream",
  "His mod pushes an interface timer, which the shimmer shares, to clear chat for a screenshot; as ChatGPT confirms, it's the simplest way.",
  "EVIDENTIAL", DREAM_R2,
  "You don't have to take my word for it though, here's me asking Chat GPT the simplest way to fade chat in a Minecraft mod",
  "15 October 2026, a Reddit comment (score -49). His screenshot shows ChatGPT answering 'Advance the HUD tick counter by 200'. He said the counter isn't the game's clock but a timer for things like chat fading, and that lag while saving the screenshot made the code add to it hundreds or thousands of times.")
S("b1_mod2_old", "b1_mod2", "Dream",
  "The mod isn't a new excuse: a 6 Hunters Rematch video from months before shows its bug of clearing underwater fog.",
  "EVIDENTIAL", DREAM_R2,
  "Which can't have been added recently as an \"excuse\", as this is from 6+ months ago.",
  "The same comment, with side-by-side screenshots. He adds that the tool also backs up the world and can pause the server. repeater64 agrees the tool is real ('Dream has proven pretty solidly that he does indeed have a custom screenshot system'); the question is whether it explains the old videos.")
C("b1_mod2_old_r", "b1_mod2_old", "repeater64",
  "The newer videos don't match the old ones: there chat disappears only inside the screenshot file; in the old \"uncut\" videos it vanishes from the video itself.",
  "EVIDENTIAL", REP_R,
  "stuff like chat and action bar (held item name) text does NOT vanish in the video. Even though it's gone in the screenshots.",
  "15 October 2026, on Reddit, two hours after Dream's comment. Tayler repeated the point on 17 October, adding that the same mod is used in the revival series, which scanned with no jumps, against 46 in the old videos.")
C("b1_mod2_ai", "b1_mod2", "repeater64",
  "Asked Dream's exact question in a new chat, ChatGPT gives a different method that doesn't touch the counter.",
  "EVIDENTIAL", REP_R_AI,
  "if you try asking chatGPT the exact same prompt in a new chat, it tells you a completely different simple method to hide chat that doesn't involve touching a tick counter.",
  "15 October 2026, on Reddit. Dream's own screenshot shows a steering prompt: on Minecraft 1.18, 'the ABSOLUTE SIMPLEST way... the LEAST CODE'. Others showed custom instructions can steer ChatGPT to his answer, and Dream declined to share a link to the conversation (Tayler, 17 October).")
C("b1_mod2_code", "b1_mod2", "repeater64",
  "The shimmer doesn't share the timer in normal Minecraft: Dream's mod was rewritten to push its real-world clock along with it, exactly the code you'd need to hide cuts.",
  "EVIDENTIAL", REP_R,
  "In the mod he sent us, this utility method is modified to, rather than returning system time, return a value that would jump by the same amount that the GUI tickCount jumps by.",
  "15 October 2026, on Reddit, about the mod Dream sent the investigators. In vanilla Minecraft that method returns the computer's clock, which the shimmer runs on.")
C("b1_mod2_back", "b1_mod2", "Tayler",
  "A loop that only adds can't make the clock go backwards, yet it drops right before the big stunts like the boat clutch.",
  "INFERENTIAL", TAYLER_AI,
  "a loop that only adds to a number can never make the timer go backwards. Yet, right before the major stunts, like the boat clutch, the timer completely resets and drops down.",
  "17 October 2026. She also asks why a mod would push a timer to clear chat when Minecraft has a setting that hides chat instantly." + AC)
S("b1_mod2_concede", "b1_mod2_back", "Dream",
  "Dream himself left the backward jumps unexplained, though he says there are simple answers.",
  "EVIDENTIAL", DREAM_R2,
  "There's obviously more to it that I'm sure will be explored in the future (by me, or by them), like why did it decrease a couple times, why such large differences, and other stuff.",
  "15 October 2026, in his own comment, two days before Tayler's video; it goes on 'There are surprisingly simple answers in most cases.' None had been given by 29 October.")
C("b1_ingots", "b1_mod", "Tayler",
  "A screenshot mod can't explain the editing slips: the old footage shows Dream taking the same six iron ingots out of a furnace twice.",
  "EVIDENTIAL", TAYLER_AI,
  "It doesn't explain how Dream pulled the exact same six iron ingots out of a furnace twice.",
  "17 October 2026." + AC)
C("b1_breaks", "b1", "Dream",
  "The real cuts are bathroom breaks and crashes, hidden when that was easy; the videos promised the full run, not a raw file.",
  "DEFINITIONAL", DREAM_R1,
  "Also there are some genuine cuts, from someone having to go to the bathroom or a server crash or whatever else. The point of those videos is for the FULL RUNS, not literally a raw file.",
  "The same Reddit comment, 10 October 2026. It goes on: such cuts 'just get cut out if there is an easy way to do it without it ruining the flow of the full video or being \"suspicious\". The same way I disguise cuts in the main video.'")
S("b1_breaks_desc", "b1_breaks", "Dream",
  "His 2021 uncut upload called itself live-stream-like footage and mentioned pauses.",
  "DEFINITIONAL", "https://www.youtube.com/watch?v=qvcnEsA2mL8",
  "There are points in this video where I check my phone, or I check my discord or something. Sometimes if I'm standing still or something that might be why. There's also a short pause while there was a bug.",
  "The video's 2021 description. Another uncut upload has a chapter titled 'Break/server pause'.")
C("b1_breaks_r", "b1_breaks", "scriptline",
  "Breaks push the game clock forward, but these cuts jump it backward at key moments.",
  "INFERENTIAL", SCRIPTLINE,
  "if you leave the game and are just on the menu, your tick counter continues going up. So when you rejoin, it'll always be higher than when you left off. But that's clearly not what's going on here.",
  "11 October 2026. He accepts some cuts for breaks are reasonable." + AC)
C("b1_leftout", "b1", "Dream",
  "The video left out answers he gave the investigators months earlier, which he says disprove 95% of it.",
  "INFERENTIAL", DREAM_R1,
  "Seems odd to completely leave out direct counters that I gave months ago, but imo they objectively prove wrong and ruin 95% of what's said in the video, so it makes sense to me nothing was included lmao",
  "10 October 2026, on Reddit.")
C("b1_leftout_r", "b1_leftout", "repeater64",
  "They knew those answers and held them for a second video, because their investigation found he was still lying.",
  "INFERENTIAL", REP_R,
  "Our thorough investigation came to the conclusion that Dream was still lying despite these explanations.",
  "15 October 2026, on Reddit. masterwyatt's part 2 and the investigators' full document are to give the details; neither was out by 29 October.")
C("b1_skeppy", "b1", "Skeppy",
  "The Manhunts he played in were real.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=F_-N5AVFzBY",
  "Speaking from my experience when I was there, yeah, it was authentic.",
  "15 October 2026. He played in two Manhunts, can't judge the ones analysed, and called some of the evidence 'hard to argue with'." + AC)

# ---- 6. The world records (Oct 2026) ---------------------------------------------
C("d1", "root", "MrMangoHands",
  "Dream's 2020 world records were spliced: in his first 1.14 record the stronghold loaded in half a second, against over 1.2 seconds in all 47 runs he streamed weeks later.",
  "EVIDENTIAL", MANGO,
  "the shortest time to finish stronghold generation from any of these 47 streamed runs is over 1.2 seconds compared to half a second for his first world record and like 2/3 of a second for his next",
  "23 October 2026, 'Dream Cheated Every World Record'. He was a leaderboard moderator from February 2021. The timing covers the two 1.14 records of March 2020; he challenges anyone to find a streamed run under one second." + AC)
C("d1_valid", "d1", "Dream",
  "His record runs were valid, played on unmodified Minecraft with no mods.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=dtZviABbSIA",
  "For anyone watching any of my old \"professional speedruns\", this run was officially valid and was world record for a short period of time! These runs were done on vanilla Minecraft/OptiFine and did not have any mods.",
  "The record video's original description, written after the 2020 ruling and kept by an archive channel.")
C("d1_removed", "d1_valid", "speedrun.com",
  "Not any more: the 1.15 record was taken off the leaderboard and is listed as rejected.",
  "EVIDENTIAL", "https://www.speedrun.com/mc/runs/mk3v37lz",
  "This run has no purpose being here anymore",
  "The leaderboard's rejection note on the run. MrMangoHands says the team pulled it in May 2021.")
C("d1_valid_r", "d1_valid", "Eliotex",
  "\"No mods\" misses the point: the 1.15 record's opening shows a different world from the one it claims, suggesting a splice.",
  "EVIDENTIAL", ELIOTEX,
  "The preview shows a forest, but when you load up the seed from Dream's previous world, you spawn on a little island without any trees nearby.",
  "23 October 2026, from the speedrun investigation team. His firmest point: a sprinting field of view in the first frame while standing at spawn with three hunger shanks. He grants contrived non-splice ways to get the preview ('this may not be a direct proof of him splicing'). Dream has not responded." + AC)
S("d1_hidden", "d1", "Christopher Norton",
  "Dream made his record videos private just before this, which makes no sense if they're real: they'd be his best evidence.",
  "INFERENTIAL", "https://www.youtube.com/watch?v=BpRGypK6dBE",
  "Maybe he has a reason, but if those runs are real, they're the best evidence he has. Hiding them is the last thing you'd do.",
  "28 October 2026. The fan wiki Wikitubia dates the 1.15 record going private to 22 October, 'for unknown reasons'. Checked 29 October: all three 2020 record videos are private, copies by others remain, and his 2025 practice streams are still public, so 'he hid every speedrun' overstates it." + AC)

# ---- 7. A private admission (Mar - Oct 2026) -------------------------------------
C("aimbot", "root", "Karl Jobst",
  "Dream privately admitted to him using an aimbot in PvP after his wrist injury.",
  "EVIDENTIAL", KARL_OCT,
  "Now, this I have a problem with because Dream admitted to me that he did use Aimbot.",
  "29 October 2026; he shows Dream's message on screen. It concerns Dream's PvP after he broke his wrist sparring in April 2025 and had surgery, not the 2020 duel. In March Karl had pointed to a visible aim 'wiggle' in Dream's 2026 fights." + AC)
S("aimbot_msg", "aimbot", "Dream",
  "In the message he wrote that he used aimbot after his surgery so he could keep making videos while in a cast.",
  "EVIDENTIAL", KARL_OCT,
  "I did use aimbot after my surgery because I was worried about being able to make videos at all during the time and I was in a clunky cast for months on my mouse hand.",
  "A Discord message dated 3 March 2026, shown in Karl Jobst's video at about 37:50.")
C("aimbot_dream", "aimbot", "Dream",
  "Whether he used an aimbot in practice with a broken wrist, never in events or competition, is irrelevant, and linking it to Technoblade is dumb.",
  "NORMATIVE", DREAM_R1_AIM,
  "Whether or not I used aimbot during practice while my wrist was broken, not in any events or competition, is pretty much irrelevant and pointless to acknowledge from my POV. Any extrapolation from that possibility to techno is dumb.",
  "10 October 2026, on Reddit, before Karl's video: neither a denial nor an admission.")
C("aimbot_dream_r", "aimbot_dream", "Dream",
  "His own earlier message says it wasn't only practice: he used it to keep making videos and to play with friends who \"don't care and/or know\".",
  "EVIDENTIAL", KARL_OCT,
  "the alternative was no videos or streams for months, and I just play with my friends pretty much who don't care and/or know.",
  "The 3 March 2026 message, as shown by Karl Jobst on 29 October.")

# Pictures of sources, shown under the quote in the claim panel: the
# screenshots themselves, where the words are in an image.
IMAGES = {
    "b1_mod2": "https://i.imgur.com/WriBq4M.png",          # Dream's ChatGPT screenshot
    "aimbot_msg": "https://i.redd.it/l3mgfgefxgyh1.jpeg",  # the 3 March 2026 message
}

# ---- Emit ----------------------------------------------------------------------
def esc(s):
    return s.replace("\\", "\\\\").replace('"', '\\"')

ids = {}
lines = []
def step(*fields):
    lines.append("steps {\n  action {\n    username: \"guest\"")
    for k, v in fields:
        if isinstance(v, int):
            lines.append(f"    {k}: {v}")
        elif v:
            lines.append(f"    {k}: \"{esc(v)}\"")
    lines.append("  }\n}")

keys = [n[0] for n in N]
assert len(keys) == len(set(keys)), "duplicate keys"
assert set(IMAGES) <= set(keys), set(IMAGES) - set(keys)
valid_bases = {"", "EVIDENTIAL", "AUTHORITY", "DEFINITIONAL", "NORMATIVE", "INFERENTIAL"}
for n in N:
    assert n[5] in valid_bases, (n[0], n[5])
    assert n[2] is None or n[2] in keys[:keys.index(n[0])], ("anchor must come first", n[0], n[2])

next_id = 1
lines.append('name: "DreamCaseSourced"\n')
lines.append("# Allegations that Dream cheated and faked Minecraft content, 2020-2026, entered")
lines.append("# by one reader in the voices of the people who said each thing. Every claim")
lines.append("# carries the page it came from and the words on it; the sources were opened")
lines.append("# on 2026-10-29. Much of the 2026 material exists only as YouTube videos, quoted")
lines.append("# from auto-captions where marked.")
lines.append("#")
lines.append("# Generated by generators/gen_dream_sourced.py; edit the data there and rerun it.\n")
step(("event_type", "CREATE_USER"))
for key, kind, anchor, voice, text, basis, url, excerpt, desc in N:
    if kind == "root":
        step(("event_type", "CREATE_DEBATE"), ("debate_topic", text), ("voice", voice),
             ("source_url", url), ("source_excerpt", excerpt), ("source_image_url", IMAGES.get(key, "")))
        step(("event_type", "JOIN_DEBATE"), ("debate_id", 1))
        step(("event_type", "ENTER_DEBATE"), ("debate_id", 1))
        ids[key] = next_id
        next_id += 1
        continue
    target = ids[anchor]
    step(("event_type", "GO_TO_CLAIM"), ("claim_id", target))
    if kind == "support":
        step(("event_type", "ADD_CHILD_CLAIM"), ("claim_text", text), ("claim_description", desc),
             ("voice", voice), ("basis", basis), ("source_url", url), ("source_excerpt", excerpt),
             ("source_image_url", IMAGES.get(key, "")))
    else:
        step(("event_type", "START_CHALLENGE_CLAIM"))
        step(("event_type", "ADD_CLAIM_TO_BE_CHALLENGED"), ("claim_id", target))
        step(("event_type", "OPEN_ADD_CHALLENGE"))
        step(("event_type", "SUBMIT_CHALLENGE_CLAIM"), ("claim_text", text), ("challenge_description", desc),
             ("voice", voice), ("basis", basis), ("source_url", url), ("source_excerpt", excerpt),
             ("source_image_url", IMAGES.get(key, "")))
    ids[key] = next_id
    next_id += 1

total = next_id - 1
lines.append("steps {")
lines.append(f'  expectations {{ check_type: "COLLECTION_SIZE" expected_count: {total} }}')
lines.append("}")
io.open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")

# The sources, grouped by page, for a reader checking the case.
by_url = {}
for key, kind, anchor, voice, text, basis, url, excerpt, desc in N:
    by_url.setdefault(url, []).append((voice, excerpt))
md = ["# Sources behind DreamCaseSourced.pbtxt", "",
      "Every page below was opened and quoted on 2026-10-29 by four research passes, one per strand: "
      "the 2020 speedrun case, the 2026 Manhunt hidden-cuts investigation, the 2026 Technoblade duel theory, "
      "and credibility and context. Quotes from YouTube videos are YouTube's automatic captions unless the "
      "claim's description says otherwise. Reddit comments were read on 2026-10-30 through the Arctic Shift "
      "archive of Reddit (reddit.com was blocked) and are quoted from the original text. "
      "Nothing here is the reader's own claim.", ""]
for i, (url, uses) in enumerate(by_url.items(), 1):
    voices = ", ".join(sorted({v for v, _ in uses}))
    md.append(f"{i}. {url}  ")
    md.append(f"   voice: {voices}  ")
    for _, ex in uses:
        md.append(f"   > {ex}")
    md.append("")
io.open(OUT_SOURCES, "w", encoding="utf-8", newline="\n").write("\n".join(md))

print(f"wrote {OUT}: {total} claims, {len(lines)} lines")
print(f"wrote {OUT_SOURCES}: {len(by_url)} distinct source urls")
print(f"voices: {len({n[3] for n in N})}")
