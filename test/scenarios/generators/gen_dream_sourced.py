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
N = []
def S(key, parent, voice, text, basis, url, excerpt, desc=""):
    N.append((key, "support", parent, voice, text, basis, url, excerpt, desc))
def C(key, target, voice, text, basis, url, excerpt, desc=""):
    N.append((key, "challenge", target, voice, text, basis, url, excerpt, desc))

# ---- Root ----------------------------------------------------------------------
N.append(("root", "root", None, "Karl Jobst",
    "Dream cheated on purpose, and has faked more of his Minecraft content than he admits.",
    "", KARL_2026,
    "So, yes, I do believe Dream cheated. And yes, I think it was intentional.",
    "Karl Jobst is a YouTuber known for investigating cheating in speedrunning. In 2021 he judged Dream's speedrun cheating probably accidental; in March 2026 he reversed that. "
    "Dream (Clay) is one of the biggest Minecraft creators, known for speedruns and the Manhunt series. The claim is wider than any one case: deliberate cheating, and faking beyond what Dream has owned up to. (Quote from auto-captions.)"))

# ---- Branch A: the 2020 Minecraft 1.16 speedrun --------------------------------
S("a1", "root", "Speedrun mods",
  "Dream's 2020 Minecraft 1.16 speedruns were played on a modified game, because his ender pearl trades and blaze rod drops were far luckier than the unmodified game allows.",
  "EVIDENTIAL", MOD_REPORT,
  "The only sensible conclusion that can be drawn after this analysis is that Dream's game was modified in order to manipulate the pearl barter and rod drop rates.",
  "The finding of the volunteer moderators of the Minecraft category on speedrun.com, in a report published in December 2020. They removed the run, which had been fifth on the 1.16 random-seed leaderboard.")
S("a1_rates", "a1", "Speedrun mods",
  "On six streams in October 2020, 42 of Dream's 262 piglin trades gave ender pearls and 211 of his 305 blazes dropped rods, where about 5% and 50% are expected.",
  "EVIDENTIAL", MOD_REPORT,
  "The data collected show that 42 of the 262 piglin barters performed throughout these streams yielded ender pearls, and 211 of the 305 killed blazes dropped blaze rods.",
  "The raw counts, from six consecutive livestreams. Roughly three times the normal pearl rate. Nobody disputes the counts; the fight is over what they mean.")
S("a1_odds", "a1", "Speedrun mods",
  "Even after correcting in Dream's favour for when the data stopped, which streams and runners were looked at and which items were checked, luck like his would come up for any speedrunner at most about once in 7.5 trillion.",
  "EVIDENTIAL", MOD_REPORT,
  "This is about 1 in 7.5 trillion. ... This is a loose (i.e., almost certainly an overestimate) upper bound on the chance that anyone in the Minecraft speedrunning community would ever get luck comparable to Dream's",
  "The often-quoted 1 in 7.5 trillion. It is an upper bound on anyone ever being this lucky, not the chance that Dream is innocent. The raw pearl-only figure, before any correction, was 1 in 177 billion.")
S("a1_geo", "a1_odds", "Speedrun mods",
  "A result this unlikely is convincing evidence of cheating unless you start out almost impossibly certain that Dream wouldn't cheat.",
  "INFERENTIAL", "https://www.youtube.com/watch?v=-MYw9LcLCb4",
  "you would need to be almost impossibly certain that dream wouldn't cheat in order for this not to be convincing evidence",
  "Moderator Geosquare's explainer video, published with the report. He also said Dream couldn't hand over his mods folder because he cleared it regularly. (Auto-captions.)")
S("a1_parker", "a1", "Matt Parker",
  "Dream's runs should not stand as world records, because even the raw odds are about ten billion times less likely than the most unlikely result he could find officially recognised in any other game.",
  "AUTHORITY", "https://www.youtube.com/watch?v=8Ko3TdPy0TU",
  "i'm not assigning guilt i'm just saying that definitively as a disinterested party i can confirm these speed runs done by dream should not be included in the list of world records",
  "Matt Parker is a mathematician who runs the Stand-up Maths channel. He looked at it as a neutral party in February 2021 and deliberately did not say whether any cheating was intentional. (Auto-captions.)")
S("a1_sim", "a1", "Karl Jobst",
  "Simulating the game's own code trillions of times never came close to Dream's rates, so odds like one in a million or one in a hundred million are simply false.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=f8TlTaTHgzo",
  "this means that figures such as one in a million one in ten million or even one in a hundred million are demonstrably false",
  "December 2020. Aimed at the much smaller odds in the report Dream commissioned. (Auto-captions.)")
S("a1_admit", "a1", "Dream",
  "Dream found that he had been using a disallowed modification on about six of his Twitch streams, which he says a hired developer had added to a recording mod without his knowledge.",
  "EVIDENTIAL", PASTEBIN,
  "After considering this, I ended up finding out that I HAD actually been using a disallowed modification during ~6 of my live streams on Twitch.",
  "His statement of May 2021, after months of denial. He said the developer had copied the boosted drop rates from the servers he uses for challenge videos into a mod he ran while streaming.")
S("a1_closed", "a1_admit", "Speedrun mods",
  "After Dream's admission the moderators considered the question of the run settled and said nothing further.",
  "EVIDENTIAL", "https://www.vice.com/en/article/huge-minecraft-streamer-dream-embroiled-in-bizarre-speedrunning-scandal/",
  "We are happy the run's validity is no longer under contention, we have no comment on anything further,",
  "A message to Vice, June 2021.")
C("a1_cherry", "a1", "Dream",
  "The moderators' sample was cherry-picked, because it starts when his luck looked unusual and stops as soon as he hit his 19-minute personal best.",
  "INFERENTIAL", "https://web.archive.org/web/20210106221537/https://dotesports.com/streaming/news/dream-minecraft-speedrun-controversy-a-history-of-events",
  "Rather than taking a truly random sample, the sample starts when the luck seems off, and the sample ends as soon as I hit a personal best goal of 19 minutes",
  "Dream's first defence, late 2020, quoted by Dot Esports. He later conceded, but this was his argument at the time.")
C("a1_cherry_r", "a1_cherry", "Speedrun mods",
  "The report already corrects for the most biased possible stopping point and for a thousand runners being examined, so any cherry-picking is priced in, in Dream's favour.",
  "INFERENTIAL", MOD_REPORT,
  "To account for the stopping rule, we will correct for the worst possible (most biased) stopping rule.",
  "The 'stopping rule' is the point where the data ends. Stopping at a lucky high point makes data look luckier, so the report assumes the worst possible one.")
C("a1_photo", "a1", "Photoexcitation",
  "The moderators' odds are far too extreme, because they ignored that runners stop trading once they have enough pearls and misapplied some corrections; the real figure is nearer 1 in 10 million.",
  "EVIDENTIAL", PHOTO,
  "I identify two major issues with the MST Report: it does not account for stopping bartering after a successful trade and it incorrectly applies some bias corrections.",
  "Photoexcitation is an anonymous self-described astrophysicist. 'Dream commissioned this report and provided direct feedback, but was not a coauthor.' Its own abstract still named cheating, intentional or not, as an obvious possibility.")
C("a1_photo_r", "a1_photo", "Speedrun mods",
  "The 1 in 10 million figure is calculated wrongly and answers a different question, because stopping after one run's pearls is not a real stop: the trades simply carry on into the next run.",
  "INFERENTIAL", MOD_REBUTTAL,
  "its \"1 in 10 million\" calculation is both invalid and not directly comparable to the \"1 in 7.5 trillion\" number from the moderator report.",
  "The moderators' rebuttal, December 2020 (text from archive.org's copy).")
C("a1_photo_r2", "a1_photo", "Speedrun mods",
  "Even if Dream's expert were right, 1 in 10 million would still mean a 99.99999% chance that Dream cheated.",
  "INFERENTIAL", MOD_REBUTTAL,
  "Even if Dream's response calculation were done correctly, the 1 in 10 million posterior probability would not be directly comparable to the 1 in 7.5 trillion figure and would still imply a 99.99999% chance of Dream cheating.",
  "The same rebuttal. Under either number, chance is not a believable explanation.")
C("a1_photo_self", "a1_photo", "Photoexcitation",
  "Even in the best interpretation, nobody's luck should be as strong as Dream's, so the drop rates were very likely modified.",
  "EVIDENTIAL", "https://archive.org/details/agreements-counterarguments-dream_202106",
  "Even in the best-possible interpretation, the probability that any speedrunner had any streak of luck as strong as Dream's is extremely low, providing significant evidence that the ender pearl and/or blaze rod drop probabilities were modified",
  "Dream's own expert, in a January 2021 follow-up partly commissioned by Dream, after reading the moderators' rebuttal. He still put the odds nearer 1 in 100 million than 1 in 7.5 trillion.")
C("a1_all11", "a1", "Photoexcitation",
  "Counting all eleven of Dream's 1.16 streams instead of only the last six, there is no significant evidence that the game was modified.",
  "EVIDENTIAL", PHOTO,
  "If these are included in the analysis and the bias corrections applied, there is no significant evidence that the game was modified.",
  "From the same December 2020 report.")
C("a1_all11_r", "a1_all11", "Speedrun mods",
  "Adding the earlier streams proves nothing, because Dream is only accused of using a modified game on the last six; the normal streams just dilute the data.",
  "INFERENTIAL", MOD_REBUTTAL,
  "This result is expected and meaningless, as Dream is only accused of using a modified game for the last 6 streams; including all streams dilutes the data, yielding inconsistent results.",
  "")
C("a1_messenger", "a1", "MinecrAvenger",
  "MinecrAvenger, the speedrunner who first gathered the data against Dream, admitted in 2022 to splicing one of his own record runs.",
  "EVIDENTIAL", "https://www.pcgamer.com/minecraft-world-record-speedrunner-who-exposed-dream-as-a-cheat-is-exposed-as-a-cheat/",
  "The splice two years ago is true and I apologize for that to [the speedruning community] and to you.",
  "PC Gamer, December 2022: he was banned over impossible chest loot and a spliced run. Dream raised this in his 2023 video. It speaks to the messenger; the 2020 finding rested on the moderators' statistics.")
C("a1_messenger_r", "a1_messenger", "Dream",
  "Dream himself said the moderators had been mostly right, so who first collected the data doesn't change the outcome.",
  "EVIDENTIAL", PASTEBIN,
  "I felt really terrible for the mods because I dragged them through the mud even though they were mostly right.",
  "From his May 2021 admission, where he also apologised to anyone he had let down.")

# ---- The question of intent ----------------------------------------------------
C("intent", "root", "Dream",
  "Dream cheated unintentionally on the speedrun he submitted in 2020, and the moderators were right to remove it.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=18jB0zQysgg",
  "I did unintentionally cheat on a speedrun that I officially submitted to the leaderboards in 2020 and the speedrun mods were completely in the right for taking it down",
  "His December 2023 video 'The Truth'. He also said the cheating had nothing to do with his Manhunt videos, and that he has donated over $50,000 to speedrunning. (Auto-captions.)")
S("intent_karl21", "intent", "Karl Jobst",
  "After interviews, message logs and testimony from the developer, Dream's account is probably true and he most likely did not know the rates were modified.",
  "AUTHORITY", "https://www.youtube.com/watch?v=G3Yzk-3SZfs",
  "in my opinion it is definitely more likely that he really didn't know his drop rates and barter rates were modified",
  "Karl Jobst in June 2021, after his own investigation. He held Dream fully responsible and praised the moderators. He reversed this view in March 2026. (Auto-captions.)")
C("intent_r", "intent", "Karl Jobst",
  "Dream's 2020 cheating was on purpose, not the accident he describes, because his later parkour streams show a blatant auto-jump and his explanations contradict his own footage.",
  "AUTHORITY", KARL_2026,
  "Dream cheated on purpose.",
  "The description of Karl Jobst's March 2026 video. He says he spoke with Dream before publishing. He also says he sees no convincing proof that Dream cheated in the Minecraft Championship parkour events themselves.")
S("mcc", "root", "Sportskeeda",
  "Resurfaced clips of Dream's 2020 Minecraft Championship parkour run led creators to accuse him of using an auto-jump macro, because his jumps were timed perfectly to the edge every time.",
  "EVIDENTIAL", "https://www.sportskeeda.com/minecraft/popular-streamer-dream-faces-backlash-old-video-alleged-cheating-minecraft-championship",
  "Creators and gamers are criticising the perfect one-pixel jumps and consistent timing across different rounds, hinting at alleged automation or the use of macros.",
  "February 2026. Dream made no statement but posted a parkour clip of himself. Dart Spark followed in April with 'More PROOF Dream Cheated at Parkour!'.")
C("mcc_r", "mcc", "Karl Jobst",
  "The loudest parkour accusers are clearly biased, and some of their arguments are inconsistent or self-serving.",
  "INFERENTIAL", KARL_2026,
  "I'm just pointing out the fact that the people who were so vocal about Dream cheating are clearly biased and some of their arguments are inconsistent or self-serving.",
  "From the same video in which Karl concludes Dream cheated on purpose. He believes Dream's 2023 parkour streams show auto-jump, but sees no convincing proof for the Championship events. (Auto-captions.)")
S("aimbot", "root", "Karl Jobst",
  "Dream privately admitted to Karl Jobst that he used an aimbot in PvP after his wrist injury, so Dream's attacks on the statistics about his 2026 streams are dishonest.",
  "EVIDENTIAL", KARL_OCT,
  "Now, this I have a problem with because Dream admitted to me that he did use Aimbot.",
  "29 October 2026, 'Dream Needs To Stop Lying'. A claim about a private conversation that can't be checked independently. It concerns Dream's 2026 streams after his late-2025 wrist surgery, not the 2020 Technoblade duel. (Auto-captions.)")

# ---- Branch B: the 2026 "uncut Manhunt" investigation --------------------------
S("b1", "root", "Dart Spark",
  "Dream's three original \"uncut\" Manhunt videos contain 46 hidden cuts, because the game's running clock, read from the hunger bar, jumps 46 times where continuous footage would rise steadily.",
  "EVIDENTIAL", DART,
  "New evidence was just discovered that suggests Dream cuts his supposedly uncut Manhunts in order to fake them. And not just once or twice, but 46 different times across the three original videos.",
  "Dart Spark's video of 9 October 2026 (1h42m, 8.8 million views), with Angle, repeater64, Fanda857 and masterwyatt. Manhunt is Dream's series in which he speedruns while friends hunt him. Dart Spark is a known Dream critic. (Auto-captions.)")
S("b1_method", "b1", "Dart Spark",
  "The hunger bar works as a hidden stopwatch, because when a player has no saturation its icons jolt in a pattern seeded by the game's tick counter, so a short run of jolts gives back the exact game time.",
  "EVIDENTIAL", DART,
  "So, what is the seed that's used for the random hunger bar jolt pattern? Well, it turns out to be none other than the value of the tick counter.",
  "The game's tick counter rises 20 times a second while the game runs. If the clock read from the screen jumps, footage is missing. Free tools to check it were published with the video. (Auto-captions.)")
S("b1_valid", "b1", "Dart Spark",
  "The method can be trusted, because it shows one unbroken line on a livestreamed Manhunt and finds exactly the known cuts in a Manhunt that was later revealed to be faked.",
  "EVIDENTIAL", DART,
  "What we're looking at here is a manhunt by Feinberg which was live streamed and obviously isn't going to have any hidden cuts in it. And we see exactly what we'd expect, a single straight unbroken line.",
  "Two control tests. The creators' names come from auto-captions and may be misspelled.")
S("b1_replic", "b1", "Erdgeist After Dark",
  "The method and Dart Spark's data hold up, because a tool rebuilt from Minecraft's own code gave the same game time for all 40 sampled readings.",
  "EVIDENTIAL", ERDGEIST,
  "I took 40 random readings from Dart's published data and my tool returned the same tick count for all 40. So this does two things. It validates that my tool works and it also validates the result of Dart Spark.",
  "22 October 2026. The only independent rebuild found; a small channel. (Auto-captions.)")
S("b1_numbers", "b1", "Dart Spark",
  "The 5 Hunters \"uncut\" video shows less than half of the game, because it has 14 cuts and the game ran over 4 hours 45 minutes for a video 2 hours 13 minutes long.",
  "EVIDENTIAL", DART,
  "That brings our total up to 14 cuts. The video that was delivered to our screens was 2 hours and 13 minutes long. And yet the true time spent was upwards of 4 hours and 45 minutes.",
  "The other two: the 5 Hunters Finale Rematch with 21 cuts (1h46m of video, about 8h18m of game), and the 4 Hunters Finale with 11 cuts (3h of video, about 7h50m). The counts are also published at dartjs.com/joltcracker/dream. (Auto-captions.)")
S("b1_splice", "b1", "Dart Spark",
  "Some footage was spliced in from a different session, because after a chest closes the game clock jumps by about three and a half hours.",
  "EVIDENTIAL", DART,
  "he has an absolutely unbelievable uptime of 3 and 1/2 hours. No, this is not a bug in the scan. This is real.",
  "He adds that at that clock time Dream was elsewhere in the same video, roaming a desert. A break or a crash cannot move the clock like that. (Auto-captions.)")
S("b1_clues", "b1", "Dart Spark",
  "The cuts show up without the hunger bar too: an enchanted axe's shimmer jumps, a furnace smelts more than the time shown allows, and the boss bar looks pasted in.",
  "EVIDENTIAL", DART,
  "The enchantment glint on his axe just completely jumps as he closes his inventory. This part's highlighted, and now suddenly this part is instead. I mean, it's a pretty blatant cut.",
  "The furnace check found about 9.3 seconds of missing footage. (Auto-captions.)")
S("b1_clutch", "b1", "repeater64",
  "The famous boat clutch was not one genuine attempt, because the two hotbar displays on screen fall out of sync, which the game's code makes impossible in one continuous take.",
  "EVIDENTIAL", DART,
  "I dived through the game code for this version of Minecraft and found that the two representations of the hotbar render based on the exact same object, making it completely impossible for them to be out of sync",
  "repeater64 is a plugin developer, speaking in Dart Spark's video. He also withdrew one of his own 2022 claims after finding it was a reproducible bug. Dream defended the clutch as real on a 2021 livestream. (Auto-captions.)")
S("b1_retry", "b1", "Dart Spark",
  "The original Manhunt series was faked by playing several attempts and secretly cutting out the failed ones, while the newer series is probably real, because its four uncut videos scanned clean.",
  "INFERENTIAL", DART,
  "I believe the original Manhunt series was faked by doing multiple attempts and secretly cutting out the failed ones, while the new series is most likely real.",
  "Dart Spark's conclusion. He allows that Dream may simply have become better at hiding cuts. (Auto-captions.)")
S("b1_retry_prod", "b1_retry", "Dream",
  "Early deaths in a Manhunt don't count as a run: the seed is dropped and the group moves on to the next one, so viewers never see those failures.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=km1IlBAZJH0",
  "So these deaths at the beginning don't actually count as a run. The seed is lost and we move down the list of seeds to the next seed.",
  "Dream's January 2026 production video, 'How Minecraft Manhunt is Made'. He also disclosed pre-scouted seeds and cash for hunters' early kills. It covers deaths at the start only, not retries in the middle of a run, and says it doesn't cover the editing.")
C("b1_shot", "b1", "Dream",
  "The glitches are not cuts: a mod that takes screenshots to mark moments for editing briefly hides the interface, which explains the fog, chat, tooltip and boss-bar jumps.",
  "EVIDENTIAL", DREAM_REDDIT_READ,
  "I have a mod that takes screenshots when I have my trigger in order to make the editing process easier by marking key moments or things that might need to get editing attention.",
  "Dream's Reddit reply of about 10 October 2026. The original comment could not be opened; these words are from a video that reads it aloud, via auto-captions ('when I have my trigger' probably reads 'hit' in the original).")
C("b1_shot_r", "b1_shot", "scriptline",
  "A screenshot mod can't explain the jumps, because the shimmer animation runs on the computer's clock rather than the game's, and a screenshot mod has no reason to touch either.",
  "EVIDENTIAL", SCRIPTLINE,
  "The glint animation derives its time value from Java's system.nanoTime. While the value returned by this function will depend on your machine, there is absolutely no means or reason for this value to ever be influenced by a mod.",
  "11 October 2026. He wrote his own screenshot mod to test it; the shimmer did not jump. He can't see Dream's actual mod. (Auto-captions.)")
C("b1_breaks", "b1", "Dream",
  "A few real cuts don't make the videos fake, because they promised the full runs rather than a raw file, and breaks for the bathroom, food or a server crash were trimmed.",
  "DEFINITIONAL", DREAM_REDDIT_READ,
  "Also, there are some genuine cuts from someone having to go to the bathroom or a server crash or whatever else. The point of those videos is for the full runs, not literally a raw file",
  "The same Reddit reply, read aloud. He called critiquing the 'unedited' name 'kind of silly'. (Auto-captions.)")
S("b1_breaks_desc", "b1_breaks", "Dream",
  "Dream presented the uncut video as extended, live-stream-like footage and noted pauses such as a bug and checking his phone.",
  "DEFINITIONAL", "https://www.youtube.com/watch?v=qvcnEsA2mL8",
  "There are points in this video where I check my phone, or I check my discord or something. Sometimes if I'm standing still or something that might be why. There's also a short pause while there was a bug.",
  "The 2021 description of the uncut 5 Hunters video on his second channel. It also discloses a custom server and recording mod. The 4 Hunters Finale upload has a chapter titled 'Break/server pause'.")
C("b1_breaks_r", "b1_breaks", "scriptline",
  "Breaks can't explain the key cuts, because sitting in a menu keeps the game clock rising, yet the scan shows big backward jumps at the most critical moments.",
  "INFERENTIAL", SCRIPTLINE,
  "if you leave the game and are just on the menu, your tick counter continues going up. So when you rejoin, it'll always be higher than when you left off. But that's clearly not what's going on here.",
  "He accepts that some cuts for breaks are reasonable. (Auto-captions.)")
C("b1_breaks_r2", "b1_breaks", "Tayler",
  "Bathroom breaks and crashes can't explain cuts that sit right before key moments and remove hours of game time, not minutes.",
  "INFERENTIAL", "https://www.youtube.com/watch?v=6Su57VsmN2c",
  "there's hours of in-game time missing, not minutes. So, if it was crashes or bathroom breaks, you're not going to be crashing for hours or going to the bathroom for hours.",
  "A commentary video, 12 October 2026. (Auto-captions.)")
C("b1_setup", "b1", "Dream",
  "The hunger-bar trick works for ordinary speedruns but doesn't apply cleanly to his custom server software and mods.",
  "INFERENTIAL", DREAM_REDDIT_READ,
  "It's definitely a useful trick to find cuts in speedruns and stuff, but obviously doesn't apply cleanly to custom server jars, custom mods, etc.",
  "The same Reddit reply, read aloud. He also said the investigators left out answers he had given them months earlier. (Auto-captions.)")
C("b1_setup_r", "b1_setup", "Erdgeist After Dark",
  "The method works on Dream's current setup: it reads his newest Manhunt as an honestly edited video, with every cut moving time forward and adding up to the run time he disclosed.",
  "EVIDENTIAL", ERDGEIST,
  "So, this matches what you would expect an honestly edited video to look like.",
  "On Dream's new '6 Hunters FINALE', October 2026. In Dream's favour for that video, and evidence the method reads his custom setup. About half the video couldn't be read. (Auto-captions.)")
C("b1_skeppy", "b1", "Skeppy",
  "The Manhunts Skeppy played in were authentic, from his own experience as one of the players.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=F_-N5AVFzBY",
  "Speaking from my experience when I was there, yeah, it was authentic.",
  "15 October 2026. He played in two Manhunts. He says he can't judge the episodes Dart Spark analysed and that some of that evidence is 'hard to argue with'. (Auto-captions.)")

# ---- Branch C: the Technoblade duel theory -------------------------------------
S("c1", "root", "GlesticOfficial",
  "Dream used aim assist through much of his career, including his 2020 $100,000 duel with Technoblade, because frame-by-frame analysis showed aiming too precise to be human.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=ca-NuR3Syug",
  "Through the process, I realized that Dream had been using the same cheat for perhaps his entire career, or at least the wide majority of it. This includes his manhunts, Minecraft Championships, and worst of all– his $100,000 duel with Technoblade.",
  "February 2026. The duel, hosted by MrBeast in August 2020, was a best of ten; Technoblade won 6 to 4. Technoblade died in 2022. Glestic credits an unnamed University of Chicago statistician. "
  "In his July 2026 follow-up he says himself that the evidence is not conclusive ('Was the evidence conclusive? And the unfortunate answer is no.'): about six minutes of footage, and professional players could reproduce Dream's swings after hundreds of tries. He still believes it from Dream's record.")
S("c1_stat", "c1", "GlesticOfficial",
  "Dream landed a perfectly timed hit on all 29 of his close-range weapon swaps in the duel, against 7 of 29 for Technoblade, where 20 to 40 percent is normal for top players.",
  "EVIDENTIAL", GLESTIC_DUEL,
  "Guess how many tick perfect swings Dream had out of 29 swaps. And the answer is every single one.",
  "July 2026, 'Investigating Dream's 100K MrBeast Duel'. His own count from about six minutes of footage. (Auto-captions.)")
S("c1_logs", "c1", "GlesticOfficial",
  "Technoblade knew about the aim assist and beat it on purpose, by fighting defensively and stripping logs, which some old cheat clients mistook for a wall.",
  "INFERENTIAL", GLESTIC_DUEL,
  "Techno tries to play this off by blaming his ADHD, but to me it is obvious what he is really trying to do.",
  "He introduces this part as 'pure speculation'. The stripped-log effect rests on his memory of cheating himself in 2020. It went viral on X on 23 and 24 October 2026. (Auto-captions.)")
C("c1_logs_tb", "c1_logs", "Technoblade",
  "Technoblade said at the time that he hit the ground with his axe by accident while trying to raise his shield.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=5MfhpJOkCKc",
  "i try to raise my shield but accidentally uh sheer the ground or whatever you call this mechanic all i know is that i clearly did not take my adhd medicine this morning",
  "His own analysis video, August 2020. He never suggested cheating and ended it: 'we both played very well we don't have actual beef it's all video games.' (Auto-captions.)")
C("c1_logs_dream", "c1_logs", "Dream",
  "No cheat client has ever stopped aiming because of stripped logs, so the theory is impossible.",
  "EVIDENTIAL", DREAM_LOGS,
  "It's not something that's ever been in a cheat client of any kind. It's never been a thing.",
  "A Short of 25 October 2026. He called the video bogus and built on 'AI statistics', said using his late friend against him was 'pretty low', and asked people to stop for the sake of those close to Technoblade. (Auto-captions.)")
C("c1_logs_dream_r", "c1_logs_dream", "GlesticOfficial",
  "There is no clip of stripped logs disabling an aimbot because the client that did it, Astolfo, no longer exists, and the Technoblade part was always labelled a theory.",
  "EVIDENTIAL", "https://www.youtube.com/post/Ugkxy0eY2SDwJsdgYs1o8VkLLhEXiLJj6v4z",
  "We had no clips of stripped logs disabling aimbot, because the original client that made this happen (Astolfo) no longer exists.",
  "A community post of about 25 October 2026. It concedes there is no working demonstration of the effect.")
C("c1_logs_ferre", "c1_logs", "FerreMC",
  "Dream may be a known cheater, but he would never use a defective cheat that breaks on stripped logs.",
  "INFERENTIAL", "https://x.com/FerreMC3/status/2113986836255428835",
  "Dream is a known cheater but the current PvP allegations are slop. No way you believe Dream would use a defective PvP cheat that breaks with stripped logs.",
  "A Minecraft commentator, 24 October 2026.")
C("c1_dart", "c1", "Dart Spark",
  "There is no convincing evidence that Dream cheated in the duel, and the stripped-log demo clip was recorded as a favour, not as evidence.",
  "AUTHORITY", "https://www.youtube.com/post/UgkxV_GiYLsYkA2DhphCWizQxZ-BcqAGem9m",
  "I still see no convincing evidence that Dream cheated in the 100k duel and that is an opinion I have been publicly stating for months.",
  "Dart Spark, himself the author of the Manhunt hidden-cuts investigation, in a community post of about 25 October 2026.")
C("c1_karl", "c1", "Karl Jobst",
  "There is no evidence convincing enough to show that Dream cheated in the Technoblade duel.",
  "AUTHORITY", KARL_OCT,
  "I have seen no evidence that convinces me that Dream cheated in that duel.",
  "29 October 2026. A long-time Dream critic; he adds that cheating in that duel would be the worst thing Dream had done, if he had. (Auto-captions.)")

# ---- Branch D: the 2020 world records ------------------------------------------
S("d1", "root", "MrMangoHands",
  "All of Dream's world-record speedruns were spliced together from separate attempts, because timings in his own videos, such as stronghold loading, are far faster than in the runs he streamed weeks later.",
  "AUTHORITY", "https://www.youtube.com/watch?v=2lkxXfMFu2Y",
  "I was a leaderboard moderator from February 2021 (after the mods talked with dream that month) and before that a technical advisor to the team from December 2020",
  "'Dream Cheated Every World Record', 23 October 2026 (2.9 million views). The quote is his stated standing, from the description; the video has no transcript, so the loading-time detail comes second-hand from a commentator. An allegation Dream had not answered as of 28 October.")
S("d1_private", "d1", "Wikitubia",
  "Dream set his 1.15 world-record video to private on 22 October 2026, shortly after the Manhunt allegations, without giving a reason.",
  "EVIDENTIAL", "https://youtube.fandom.com/wiki/Dream",
  "On October 22, 2026, shortly after Dart Spark's Manhunt cheating allegations, he set his 1.15 WR video on YouTube to private for unknown reasons.",
  "A fan wiki. Checked on 29 October 2026: all three of his 2020 world-record videos show as private; copies uploaded by others remain.")
S("d1_norton", "d1_private", "Christopher Norton",
  "Hiding the record videos looks bad, because if those runs are real they are the best evidence Dream has.",
  "INFERENTIAL", "https://www.youtube.com/watch?v=BpRGypK6dBE",
  "Maybe he has a reason, but if those runs are real, they're the best evidence he has. Hiding them is the last thing you'd do.",
  "A commentator, 28 October 2026. In the same video he calls the Technoblade stripped-log theory nonsense. (Auto-captions.)")
S("d1_tayler", "d1_private", "Tayler",
  "Dream panicked and made every one of his Minecraft speedrun videos private right after two investigations said his records were spliced.",
  "INFERENTIAL", "https://www.youtube.com/watch?v=lKRG2mGA_Rk",
  "Dream just panicked and privated every single Minecraft speedrun on his channel.",
  "A commentary video, 25 October 2026. 'Panicked' is her reading; Dream has given no reason.")
C("d1_tayler_r", "d1_tayler", "Dream",
  "Dream's 2025 speedrun streams are still public, labelled as practice for his videos with mods that leaderboards don't allow, such as higher pearl rates.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=v0YTHmoY4TQ",
  "I am using mods that are not allowed in official leaderboard speedruns (Like increased pearl rates, zoom, no fog, etc.) This is practice for my videos, not for world records!",
  "The description of his own August 2025 stream, still public on 29 October 2026. Only the three 2020 record videos are private, so 'every single' overstates it. These 2025 runs were never record attempts.")
C("d1_valid", "d1", "Dream",
  "Dream's record runs were officially valid at the time, done on unmodified Minecraft or OptiFine with no mods.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=dtZviABbSIA",
  "For anyone watching any of my old \"professional speedruns\", this run was officially valid and was world record for a short period of time! These runs were done on vanilla Minecraft/OptiFine and did not have any mods.",
  "The original description of his 1.15 record video, kept by an archive channel; written after the December 2020 ruling. The 2026 allegation is splicing, not mods.")
C("d1_valid_r", "d1_valid", "Eliotex",
  "\"No mods\" doesn't answer the allegation, because the 1.15 record is spliced: the video's first frames show a different previous world, recovered by seed-crackers.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=VVFywSmvUL4",
  "When Dream cheated his 1.16 on accident, the investigation group was a bit sceptical about his other speedruns.",
  "Eliotex's investigation, 23 October 2026. The quote is from the description; the details come from a commentator's account of the video. An allegation.")

# ---- Branch E: the pile-on ------------------------------------------------------
C("pile", "root", "RoyaltyIsHere",
  "Some of the case against Dream is a pile-on: he was one of the most hated people online, which led to false allegations, harassment and doxxing.",
  "NORMATIVE", "https://www.youtube.com/watch?v=NZ7aiOgGFhw",
  "Dream was one of the most hated individuals on the planet at one point and that potentially led to false allegations being levied against him, harassment, doxing.",
  "22 October 2026. The same video describes Dream's pattern: deny at first, add context under pressure, and almost never admit to doing it on purpose. (Auto-captions.)")
S("pile_patches", "pile", "Patches",
  "Of 35 gameplay accusations against Dream, 4 turned out to be wrong and 17 are only alleged or still disputed, though he admitted 10.",
  "EVIDENTIAL", "https://www.youtube.com/watch?v=rWiZqBjyMEA",
  "I tracked down all 35 accusations about his gameplay and checked each one against its source. 10 he admitted. 4 were measured. 10 are still disputed. 7 are only alleged. And 4 turned out to be wrong.",
  "A count by a commentator, 13 October 2026. It cuts both ways: 14 were admitted or measured.")

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
