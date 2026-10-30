# Emits test/scenarios/DreamCaseSourced.pbtxt and DreamCaseSourced.SOURCES.md:
# the allegations that the Minecraft YouTuber Dream cheated and faked content,
# entered by one reader ("guest") in the voices of the people who actually said
# each thing. Every claim carries the page it was taken from and the words on it.
#
# Research: four passes on 2026-10-29, one per strand (the 2020 speedrun case,
# the 2026 Manhunt hidden-cuts investigation, the 2026 Technoblade duel theory,
# and credibility and context). Every page was opened. Much of the 2026 material
# exists only as YouTube videos: excerpts marked (auto-captions) are YouTube's
# automatic captions, copied as captioned. Dream's October 2026 Reddit reply
# could not be opened (reddit.com was blocked); its words come from a video that
# reads it aloud, and the description says so.
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
DREAM_REDDIT_READ = "https://www.youtube.com/watch?v=LE94mDE4Cxw"
SCRIPTLINE = "https://www.youtube.com/watch?v=lBauWJX3aAo"
ERDGEIST = "https://www.youtube.com/watch?v=fJFL-ZoxsOE"
GLESTIC_DUEL = "https://www.youtube.com/watch?v=OuwEMf8khRw"
DREAM_LOGS = "https://www.youtube.com/shorts/iSwIiTNVvWo"

# (key, kind, anchor_key, voice, text, basis, url, excerpt, description)
# kind: "root" | "support" (anchor = parent) | "challenge" (anchor = target)
# basis: one of backend/config/bases.pbtxt: EVIDENTIAL, AUTHORITY, DEFINITIONAL,
# NORMATIVE, INFERENTIAL.
#
# Shape: each branch is a thread in date order -- a point, the reply to it,
# the reply to that -- entered oldest first, so the map and the Brief read as
# the argument actually went. Claim text is one short sentence with its
# reason; context, dates and caveats live in the description.
N = []
def S(key, parent, voice, text, basis, url, excerpt, desc=""):
    N.append((key, "support", parent, voice, text, basis, url, excerpt, desc))
def C(key, target, voice, text, basis, url, excerpt, desc=""):
    N.append((key, "challenge", target, voice, text, basis, url, excerpt, desc))

AC = " (Quote from YouTube's auto-captions.)"

# ---- Root ----------------------------------------------------------------------
N.append(("root", "root", None, "Karl Jobst",
    "Dream cheated on purpose, and his explanations can't be trusted.",
    "", KARL_2026,
    "So, yes, I do believe Dream cheated. And yes, I think it was intentional.",
    "Karl Jobst investigates cheating in speedrunning. In 2021 he thought Dream's cheating was probably an accident; in March 2026 he changed his mind, and in October 2026 he published 'Dream Needs To Stop Lying'. "
    "Each branch below is one allegation, told in order: the accusation, Dream's answer, and the replies." + AC))

# ---- 1. The 2020 speedrun (Dec 2020 - Jun 2021) ---------------------------------
S("a1", "root", "Speedrun mods",
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
C("a1_cherry", "a1", "Dream",
  "The sample is cherry-picked: it starts when his luck looked odd and stops at his best time.",
  "INFERENTIAL", "https://web.archive.org/web/20210106221537/https://dotesports.com/streaming/news/dream-minecraft-speedrun-controversy-a-history-of-events",
  "Rather than taking a truly random sample, the sample starts when the luck seems off, and the sample ends as soon as I hit a personal best goal of 19 minutes",
  "Dream's first defence, December 2020, as quoted by Dot Esports.")
C("a1_cherry_r", "a1_cherry", "Speedrun mods",
  "The report already assumes the most unfavourable stopping point, so cherry-picking is accounted for.",
  "INFERENTIAL", MOD_REPORT,
  "To account for the stopping rule, we will correct for the worst possible (most biased) stopping rule.",
  "From the original report: it built in the correction Dream asked for.")
C("a1_photo", "a1", "Photoexcitation",
  "The odds are nearer 1 in 10 million, because the moderators misapplied their corrections.",
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
  "Even at the most generous reading, the drop rates were very likely modified.",
  "EVIDENTIAL", "https://archive.org/details/agreements-counterarguments-dream_202106",
  "Even in the best-possible interpretation, the probability that any speedrunner had any streak of luck as strong as Dream's is extremely low, providing significant evidence that the ender pearl and/or blaze rod drop probabilities were modified",
  "8 January 2021. Dream's own expert, after the rebuttal, in a follow-up partly paid for by Dream.")
S("a1_parker", "a1", "Matt Parker",
  "As a neutral mathematician: these runs should not count as records.",
  "AUTHORITY", "https://www.youtube.com/watch?v=8Ko3TdPy0TU",
  "i'm not assigning guilt i'm just saying that definitively as a disinterested party i can confirm these speed runs done by dream should not be included in the list of world records",
  "February 2021, Stand-up Maths. He found the odds about ten billion times less likely than any officially recognised result in any game, and deliberately said nothing about intent." + AC)
S("a1_admit", "a1", "Dream",
  "A disallowed mod was in his game on about six streams, put there by a developer without his knowledge.",
  "EVIDENTIAL", PASTEBIN,
  "After considering this, I ended up finding out that I HAD actually been using a disallowed modification during ~6 of my live streams on Twitch.",
  "May 2021, after five months of denial. He said the developer had copied the boosted drop rates from his challenge-video servers into a mod he used while streaming, and that the moderators were 'mostly right'.")
S("a1_closed", "a1_admit", "Speedrun mods",
  "With Dream's admission, the run's validity is settled.",
  "EVIDENTIAL", "https://www.vice.com/en/article/huge-minecraft-streamer-dream-embroiled-in-bizarre-speedrunning-scandal/",
  "We are happy the run's validity is no longer under contention, we have no comment on anything further,",
  "June 2021, to Vice.")

# ---- 2. Accident or on purpose? (2021 - 2026) ------------------------------------
C("intent", "root", "Dream",
  "It was an accident, and the moderators were right to remove the run.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=18jB0zQysgg",
  "I did unintentionally cheat on a speedrun that I officially submitted to the leaderboards in 2020 and the speedrun mods were completely in the right for taking it down",
  "December 2023, 'The Truth'. He also said it had nothing to do with his Manhunt videos." + AC)
S("intent_karl21", "intent", "Karl Jobst",
  "After interviews, chat logs and the developer's own account, Dream most likely didn't know.",
  "AUTHORITY", "https://www.youtube.com/watch?v=G3Yzk-3SZfs",
  "in my opinion it is definitely more likely that he really didn't know his drop rates and barter rates were modified",
  "June 2021. Karl reversed this view in 2026." + AC)
C("intent_r", "intent", "Karl Jobst",
  "It was on purpose: in Dream's 2023 parkour streams, whenever he snags on a pole he never jumps, and in 8 hours he never once jumps too late, the signs of an auto-jump macro.",
  "EVIDENTIAL", KARL_2026,
  "in every single case that he gets stuck on a pole in this way, he doesn't jump.",
  "March 2026, from Dream's November 2023 streams. Also: 'I could not find a single instance where Dream accidentally jumped too late and ran off an edge. Yes, 8 hours in a row.' He calls this 'the clearest evidence' of a macro, and says a cheater caught twice makes 'accidental' hard to believe. The description reads 'Dream cheated on purpose.' Dream has not answered it." + AC)

# ---- 3. Championship parkour (Feb - Mar 2026) -------------------------------------
S("mcc", "root", "Luna",
  "In his 2020 Championship parkour, Dream jumped at the last possible moment 225 times out of 226, too consistent for a human.",
  "EVIDENTIAL", KARL_2026,
  "tweet by Luna who claimed that Dream jumped from the last possible tick 225 out of 226 times. She also uploaded a video showing 225 tick perfect jumps in a row. The claim is that this is impossible.",
  "February 2026. Luna's post couldn't be opened; this is Karl Jobst's account of it. A 'last possible tick' jump leaves one game tick (50 milliseconds) to press jump. Sportskeeda covered the wave of accusations of 'perfect one-pixel jumps'." + AC)
C("mcc_dream", "mcc", "Dream",
  "Long runs of tick-perfect jumps are easy: he filmed himself landing 50 in a row with his feet on the desk.",
  "EVIDENTIAL", KARL_2026,
  "there's footage playing of me in the background with a hand cam hitting [music] 50 in a row with my feet both on the desk just to be silly.",
  "February 2026, a Short Dream posted in reply, as played in Karl Jobst's video." + AC)
C("mcc_karl", "mcc", "Karl Jobst",
  "The statistic fails: most counted jumps had visual cues, and at a top player's 92.7% success rate, 69 cue-less jumps in a row is only 1 in 186.",
  "EVIDENTIAL", KARL_2026,
  "The best performer was Lily, who achieved a success rate of 92.7%. With this kind of success rate, the odds of getting 69 non-visual one-tick jumps in a row is 1 in 186. Those odds aren't outrageous.",
  "March 2026. He also noted that Dream accidentally ran off an edge in that event, which an auto-jump should prevent. So the Championship case doesn't hold up for him; his own evidence of a macro is from Dream's 2023 streams (see 'Accident or on purpose')." + AC)

# ---- 4. The Technoblade duel (Feb - Oct 2026) -------------------------------------
S("c1", "root", "GlesticOfficial",
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
  "Technoblade's own analysis video, August 2020. He never suggested cheating: 'we don't have actual beef it's all video games.'" + AC)
C("c1_logs_dream", "c1_logs", "Dream",
  "No cheat client has ever stopped aiming because of stripped logs.",
  "EVIDENTIAL", DREAM_LOGS,
  "It's not something that's ever been in a cheat client of any kind. It's never been a thing.",
  "25 October 2026. He also asked people to stop using his late friend this way." + AC)
C("c1_logs_dream_r", "c1_logs_dream", "GlesticOfficial",
  "There's no demonstration because the client that did it no longer exists.",
  "EVIDENTIAL", "https://www.youtube.com/post/Ugkxy0eY2SDwJsdgYs1o8VkLLhEXiLJj6v4z",
  "We had no clips of stripped logs disabling aimbot, because the original client that made this happen (Astolfo) no longer exists.",
  "About 25 October 2026. So the effect has never been shown working.")
C("c1_dart", "c1", "Dart Spark",
  "There's no convincing evidence Dream cheated in the duel.",
  "AUTHORITY", "https://www.youtube.com/post/UgkxV_GiYLsYkA2DhphCWizQxZ-BcqAGem9m",
  "I still see no convincing evidence that Dream cheated in the 100k duel and that is an opinion I have been publicly stating for months.",
  "About 25 October 2026. Dart Spark leads the Manhunt investigation below, so this comes from a critic.")
C("c1_karl", "c1", "Karl Jobst",
  "Nothing he has seen shows Dream cheated in the duel.",
  "AUTHORITY", KARL_OCT,
  "I have seen no evidence that convinces me that Dream cheated in that duel.",
  "29 October 2026, also a long-time critic." + AC)

# ---- 5. The "uncut" Manhunts (Oct 2026) -------------------------------------------
S("b1", "root", "Dart Spark",
  "Dream's three original \"uncut\" Manhunts hide 46 cuts, found by reading the game's clock from the hunger bar.",
  "EVIDENTIAL", DART,
  "New evidence was just discovered that suggests Dream cuts his supposedly uncut Manhunts in order to fake them. And not just once or twice, but 46 different times across the three original videos.",
  "9 October 2026, with help from Angle, repeater64, Fanda857 and masterwyatt. Manhunt is Dream's series where he speedruns while friends hunt him." + AC)
S("b1_method", "b1", "Dart Spark",
  "With no saturation, the hunger icons jolt in a pattern set by the game's tick counter, so the jolts reveal the exact game time.",
  "EVIDENTIAL", DART,
  "So, what is the seed that's used for the random hunger bar jolt pattern? Well, it turns out to be none other than the value of the tick counter.",
  "The tick counter rises 20 times a second while the game runs; a jump means missing footage. The tools were published with the video." + AC)
S("b1_valid", "b1", "Dart Spark",
  "The method finds no cuts in a livestream and exactly the known cuts in a confirmed fake.",
  "EVIDENTIAL", DART,
  "What we're looking at here is a manhunt by Feinberg which was live streamed and obviously isn't going to have any hidden cuts in it. And we see exactly what we'd expect, a single straight unbroken line.",
  "His two control tests." + AC)
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
S("b1_clutch", "b1", "repeater64",
  "The famous boat clutch was not one take: a hidden cut sits seconds before it, and its two hotbar displays fall out of sync, which the game can't do.",
  "EVIDENTIAL", DART,
  "I dived through the game code for this version of Minecraft and found that the two representations of the hotbar render based on the exact same object, making it completely impossible for them to be out of sync",
  "repeater64, a plugin developer, speaking in Dart Spark's video. Dream defended the clutch as real on a 2021 livestream." + AC)
S("b1_retry", "b1", "Dart Spark",
  "The original series was faked by retrying and secretly cutting out failures; the newer series scans clean.",
  "INFERENTIAL", DART,
  "I believe the original Manhunt series was faked by doing multiple attempts and secretly cutting out the failed ones, while the new series is most likely real.",
  "His conclusion." + AC)
S("b1_prod", "b1_retry", "Dream",
  "Early deaths don't count: the seed is dropped and they move on, so viewers never see those failures.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=km1IlBAZJH0",
  "So these deaths at the beginning don't actually count as a run. The seed is lost and we move down the list of seeds to the next seed.",
  "Dream's own production video, January 2026. It covers deaths at the start, not retries in the middle of a run.")
C("b1_mod", "b1", "Dream",
  "The glitches come from a mod that takes screenshots to mark moments for editing, not from cuts.",
  "EVIDENTIAL", DREAM_REDDIT_READ,
  "I have a mod that takes screenshots when I have my trigger in order to make the editing process easier by marking key moments or things that might need to get editing attention.",
  "About 10 October 2026, on Reddit. The original comment couldn't be opened; these are his words as read aloud in a video, via auto-captions.")
C("b1_mod_r", "b1_mod", "scriptline",
  "A screenshot mod can't make the item shimmer jump, because it runs on the computer's clock, not the game's.",
  "EVIDENTIAL", SCRIPTLINE,
  "The glint animation derives its time value from Java's system.nanoTime. While the value returned by this function will depend on your machine, there is absolutely no means or reason for this value to ever be influenced by a mod.",
  "11 October 2026. He built his own screenshot mod to test it; the shimmer didn't jump." + AC)
C("b1_breaks", "b1", "Dream",
  "The real cuts are bathroom breaks and crashes; the videos promised the full run, not a raw file.",
  "DEFINITIONAL", DREAM_REDDIT_READ,
  "Also, there are some genuine cuts from someone having to go to the bathroom or a server crash or whatever else. The point of those videos is for the full runs, not literally a raw file",
  "Same Reddit reply, read aloud." + AC)
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
C("b1_setup", "b1", "Dream",
  "The trick doesn't apply cleanly to his custom server and mods.",
  "INFERENTIAL", DREAM_REDDIT_READ,
  "It's definitely a useful trick to find cuts in speedruns and stuff, but obviously doesn't apply cleanly to custom server jars, custom mods, etc.",
  "Same Reddit reply, read aloud." + AC)
C("b1_setup_r", "b1_setup", "Erdgeist After Dark",
  "An independent rebuild of the method matches Dart's data, and it reads Dream's newest Manhunt as honestly edited.",
  "EVIDENTIAL", ERDGEIST,
  "I took 40 random readings from Dart's published data and my tool returned the same tick count for all 40. So this does two things. It validates that my tool works and it also validates the result of Dart Spark.",
  "22 October 2026. Reading Dream's new 6 Hunters Finale, he found every cut moves time forward, as honest editing would: the method works on Dream's current setup, and that video comes out clean." + AC)
C("b1_skeppy", "b1", "Skeppy",
  "The Manhunts he played in were real.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=F_-N5AVFzBY",
  "Speaking from my experience when I was there, yeah, it was authentic.",
  "15 October 2026. He played in two Manhunts, can't judge the ones analysed, and called some of the evidence 'hard to argue with'." + AC)

# ---- 6. Pile-on? (Oct 2026) -------------------------------------------------------
C("pile", "root", "RoyaltyIsHere",
  "Part of the backlash is a pile-on that has produced false allegations and harassment.",
  "NORMATIVE", "https://www.youtube.com/watch?v=NZ7aiOgGFhw",
  "Dream was one of the most hated individuals on the planet at one point and that potentially led to false allegations being levied against him, harassment, doxing.",
  "22 October 2026. The same video also describes Dream's pattern of denying first and adding context only under pressure." + AC)
S("pile_count", "pile", "Patches",
  "Of 35 accusations checked, 4 were wrong and 17 are unproven, though 14 were admitted or measured.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=rWiZqBjyMEA",
  "I tracked down all 35 accusations about his gameplay and checked each one against its source. 10 he admitted. 4 were measured. 10 are still disputed. 7 are only alleged. And 4 turned out to be wrong.",
  "13 October 2026, the video's description.")

# ---- 7. The world records (Oct 2026) ---------------------------------------------
S("d1", "root", "MrMangoHands",
  "Dream's 2020 world records were spliced from separate attempts: strongholds load about twice as fast as in 47 runs he streamed weeks later.",
  "AUTHORITY", "https://www.youtube.com/watch?v=2lkxXfMFu2Y",
  "I was a leaderboard moderator from February 2021 (after the mods talked with dream that month) and before that a technical advisor to the team from December 2020",
  "23 October 2026, 'Dream Cheated Every World Record'. The quote is his stated standing; the video has no transcript, so its evidence (such as loading times faster than in Dream's later streams) is known second-hand.")
S("d1_private", "d1", "Wikitubia",
  "Dream made his record videos private on 22 October, without giving a reason.",
  "EVIDENTIAL", "https://youtube.fandom.com/wiki/Dream",
  "On October 22, 2026, shortly after Dart Spark's Manhunt cheating allegations, he set his 1.15 WR video on YouTube to private for unknown reasons.",
  "A fan wiki. Checked 29 October: all three 2020 record videos are private, copies by others remain, and his 2025 practice streams are still public, so 'he hid every speedrun' overstates it.")
S("d1_norton", "d1_private", "Christopher Norton",
  "If the runs are real they're his best evidence, so hiding them makes no sense.",
  "INFERENTIAL", "https://www.youtube.com/watch?v=BpRGypK6dBE",
  "Maybe he has a reason, but if those runs are real, they're the best evidence he has. Hiding them is the last thing you'd do.",
  "28 October 2026." + AC)
C("d1_valid", "d1", "Dream",
  "His record runs were valid, played on unmodified Minecraft with no mods.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=dtZviABbSIA",
  "For anyone watching any of my old \"professional speedruns\", this run was officially valid and was world record for a short period of time! These runs were done on vanilla Minecraft/OptiFine and did not have any mods.",
  "The record video's original description, written after the 2020 ruling and kept by an archive channel.")
C("d1_valid_r", "d1_valid", "Eliotex",
  "\"No mods\" misses the point: the 1.15 record opens on a different world, so it was spliced.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=VVFywSmvUL4",
  "When Dream cheated his 1.16 on accident, the investigation group was a bit sceptical about his other speedruns.",
  "23 October 2026. The quote is from the description; the detail is known from a commentator's account. Dream has not responded.")

# ---- 8. A private admission (Oct 2026) -------------------------------------------
S("aimbot", "root", "Karl Jobst",
  "Dream privately admitted to him using an aimbot in PvP after his wrist injury.",
  "EVIDENTIAL", KARL_OCT,
  "Now, this I have a problem with because Dream admitted to me that he did use Aimbot.",
  "29 October 2026. A private conversation that can't be checked. It concerns Dream's PvP after he broke his wrist sparring in April 2025, not the 2020 duel. In March Karl had pointed to a visible aim 'wiggle' in Dream's 2026 fights." + AC)

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
             ("source_url", url), ("source_excerpt", excerpt))
        step(("event_type", "JOIN_DEBATE"), ("debate_id", 1))
        step(("event_type", "ENTER_DEBATE"), ("debate_id", 1))
        ids[key] = next_id
        next_id += 1
        continue
    target = ids[anchor]
    step(("event_type", "GO_TO_CLAIM"), ("claim_id", target))
    if kind == "support":
        step(("event_type", "ADD_CHILD_CLAIM"), ("claim_text", text), ("claim_description", desc),
             ("voice", voice), ("basis", basis), ("source_url", url), ("source_excerpt", excerpt))
    else:
        step(("event_type", "START_CHALLENGE_CLAIM"))
        step(("event_type", "ADD_CLAIM_TO_BE_CHALLENGED"), ("claim_id", target))
        step(("event_type", "OPEN_ADD_CHALLENGE"))
        step(("event_type", "SUBMIT_CHALLENGE_CLAIM"), ("claim_text", text), ("challenge_description", desc),
             ("voice", voice), ("basis", basis), ("source_url", url), ("source_excerpt", excerpt))
    ids[key] = next_id
    next_id += 1

total = next_id - 1
lines.append("steps {")
lines.append(f'  expectations {{ check_type: "COLLECTION_SIZE" expected_count: {total} }}')
lines.append(f'  expectations {{ check_type: "USER_VIEW" claim_id: 1 username: "Karl Jobst" expected_status: "TRUE_CLAIM" }}')
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
      "claim's description says otherwise. Dream's October 2026 Reddit reply could not be opened; its words "
      "come from a video that reads it aloud. Nothing here is the reader's own claim.", ""]
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
