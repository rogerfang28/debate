# Emits test/scenarios/FauciCaseSourced.pbtxt: the Fauci / Paul dispute entered
# by one reader ("guest") in the voices of the people who actually said each
# thing, every claim carrying the page it was taken from and the words on it.
import io

import os
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "FauciCaseSourced.pbtxt")

CBS_LIVE = "https://www.cbsnews.com/live-updates/anthony-fauci-rand-paul-covid-origins-senate/"
CBS_DIARY = "https://www.cbsnews.com/news/anthony-fauci-rand-paul-diary-release/"

# (key, kind, anchor_key, voice, text, basis, url, excerpt, description)
# kind: "root" | "support" (anchor = parent) | "challenge" (anchor = target)
#
# Shape: every claim answers its parent -- a reason for it, or a reply against
# it -- and a reply sits under the claim it answers, not beside it. Each branch
# runs in date order: the charge, the answer, the answer to that.
N = []
def S(key, parent, voice, text, basis, url, excerpt, desc=""):
    N.append((key, "support", parent, voice, text, basis, url, excerpt, desc))
def C(key, target, voice, text, basis, url, excerpt, desc=""):
    N.append((key, "challenge", target, voice, text, basis, url, excerpt, desc))

# ---- Root ----------------------------------------------------------------------
N.append(("root", "root", None, "Paul",
    "Fauci was dishonest with Congress and the public about COVID-19.",
    "", "https://www.paul.senate.gov/after-silence-from-doj-dr-rand-paul-asks-u-s-attorney-to-investigate-anthony-fauci",
    "I warned Dr. Fauci of the criminal implications of lying to Congress and offered him an opportunity to recant his previous statement. Dr. Fauci's testimony is inconsistent with facts that have since come to light.",
    "Senator Rand Paul's charge, pressed since 2021. Three branches argue it (the 2021 testimony, the origin statements, the 2026 silence) and one answers it (Fauci's view that it's a vendetta). Each runs in date order."))

# ---- 1. The May 2021 testimony (2021 - 2024) ----------------------------------
S("a1", "root", "Paul",
  "Fauci's May 2021 testimony that NIH did not fund gain-of-function research at the Wuhan Institute of Virology was false.",
  "EVIDENTIAL", "https://www.paul.senate.gov/after-silence-from-doj-dr-rand-paul-asks-u-s-attorney-to-investigate-anthony-fauci",
  "Dr. Fauci's testimony is inconsistent with facts that have since come to light.",
  "The sentence under oath, 11 May 2021, which nobody disputes he said: the NIH 'has not ever and does not now fund gain-of-function research in the Wuhan Institute of Virology' (PolitiFact). Lying to Congress under oath is a federal crime, so this says Fauci knowingly said something untrue, not that he was mistaken. Paul sent criminal referrals in 2021 and 2023; no prosecution followed.")
S("a1_money", "a1", "FactCheck.org",
  "NIH money did reach the Wuhan lab: EcoHealth Alliance passed nearly $600,000 of a $3.7 million NIH grant to it, with NIH's pre-approval.",
  "EVIDENTIAL", "https://www.factcheck.org/2021/05/the-wuhan-lab-and-the-gain-of-function-disagreement/",
  "EcoHealth ultimately received $3.7 million over six years from the NIH and distributed nearly $600,000 of that total to China's Wuhan Institute of Virology, a collaborator on the project, pre-approved by NIH.",
  "May 2021. The money trail, agreed by everyone; the fight is over what the money bought.")
S("a1_def", "a1", "Paul",
  "What it bought was gain-of-function by NIH's own website, which until October 2021 defined it as research that gives a biological agent new or enhanced activity.",
  "DEFINITIONAL", "https://www.paul.senate.gov/news-dr-paul-rep-comer-send-letter-director-nih-requesting-information-removal-gain-function-nih",
  "The update removed a section on the website titled 'Gain-of-Function Research,' which defined gain-of-function research as 'a type of research that modifies a biological agent so that it confers new or enhanced activity to that agent.'",
  "The broad, plain-language definition. NIH removed it around the day the Tabak letter went out (see below). Under this reading the Wuhan mouse experiment counts.")
C("a1_p3co", "a1_def", "Fauci",
  "The definition that governs funding is the regulatory P3CO one, and under it the NIH did not fund gain-of-function research in Wuhan.",
  "DEFINITIONAL", "https://www.govinfo.gov/content/pkg/CHRG-118hhrg55830/html/CHRG-118hhrg55830.htm",
  "According to the regulatory and operative definition of P3CO, the NIH did not fund gain-of-function research at the Wuhan Institute of Virology.",
  "Fauci to the House, June 2024, the heart of his defence. He disputes no fact, only which definition applies: the rules governing federal funding use a narrow technical term, and the bat-virus work did not meet it.")
S("a1_pause", "a1_p3co", "NIH",
  "That narrow definition is the rulebook: the 2014 funding pause covered work expected to make flu, MERS or SARS viruses more dangerous, and P3CO replaced it in 2017.",
  "AUTHORITY", "https://grants.nih.gov/grants/guide/notice-files/NOT-OD-17-071.html",
  "instituting a funding pause on gain-of-function research projects that could be reasonably anticipated to confer attributes to influenza, MERS, or SARS viruses such that the resulting virus has enhanced pathogenicity and/or transmissibility",
  "Where the narrow definition comes from: a three-year pause on certain risky work, replaced in December 2017 by extra review for a specific category of pathogen.")
S("a1_collins", "a1_p3co", "Collins",
  "NIH would not have supported coronavirus research that might produce a virus more dangerous than nature's.",
  "AUTHORITY", "https://hughhewitt.com/nih-director-francis-collins-on-gain-of-function-research-at-nih-and-wuhan-lab",
  "NIH would not have supported any such research on coronaviruses, because there are risks there that you might actually end up producing a virus that has a higher danger attached to it than what nature has already come up with.",
  "The NIH director, two days after the May 2021 testimony, backing Fauci with the narrow framing.")
C("a1_nebulous", "a1_p3co", "Paul",
  "Fauci himself called gain-of-function a very nebulous term, so his flat denial rested on a definition he admits is unsettled.",
  "DEFINITIONAL", "https://www.foxnews.com/politics/rand-paul-anthony-fauci-senate-hearing-gain-of-function.print",
  "Gain of function is a very nebulous term that outside parties have worked on assigning a more precise definition to.",
  "Fauci's own words, which both sides quote against each other: Paul reads an admission the denial was slippery; Fauci's side reads the reason a precise regulatory definition is the only fair test.")
C("a1_ebright", "a1_p3co", "Ebright",
  "The work fails even the narrow test: the grant documents show the denial of potential pandemic pathogen enhancement in Wuhan was untruthful.",
  "AUTHORITY", "https://www.newsweek.com/fauci-untruthful-congress-wuhan-lab-research-documents-show-gain-function-1627351",
  "The documents make it clear that assertions by the NIH director, Francis Collins, and the NIAID director, Anthony Fauci, that the NIH did not support gain-of-function research or potential pandemic pathogen enhancement in Wuhan are untruthful.",
  "September 2021. Richard Ebright is a Rutgers molecular biologist and the most-cited scientific critic. 'Potential pandemic pathogen enhancement' is the P3CO category itself.")
C("a1_never_lied", "a1", "Fauci",
  "Fauci has never lied before Congress and does not retract the May 2021 statement.",
  "EVIDENTIAL", "https://www.factcheck.org/2021/07/scicheck-fauci-and-paul-round-2/",
  "Sen. Paul, I have never lied before the Congress, and I do not retract that statement.",
  "July 2021, to Paul's face, in the exchange where Fauci also told Paul he did not know what he was talking about.")
# October 2021: NIH's own letter, the second route to the charge.
C("b1", "a1_never_lied", "Ebright",
  "NIH's own October 2021 letter shows that Collins and Fauci lied to Congress, to the press and to the public.",
  "AUTHORITY", "https://www.washingtonexaminer.com/news/801375/bombshell-nih-letter-on-bat-coronavirus-research-reveals-faucis-big-lie-professor-says/",
  "Collins and Fauci lied to Congress, lied to the press, and lied to the public.",
  "October 2021. A route to the charge that doesn't depend on the definition fight: NIH's own description of what the funded work did.")
S("b1_tabak", "b1", "Tabak",
  "The letter says that in a limited experiment, mice infected with a modified bat coronavirus became sicker than those infected with the original.",
  "EVIDENTIAL", "https://www.documentcloud.org/documents/21674679-tabak-letter-to-comer-oct-20-2021",
  "In this limited experiment, laboratory mice infected with the SHC014 WIV1 bat coronavirus became sicker than those infected with the WIV1 bat coronavirus.",
  "Lawrence Tabak, NIH's principal deputy director, to Rep. Comer, 20 October 2021. Nobody disputes a word of it; both sides say it vindicates them.")
S("b1_term", "b1", "HHS",
  "NIH foresaw this: the grant required EcoHealth to report at once any virus that grew more than ten times faster than the original.",
  "EVIDENTIAL", "https://oversight.house.gov/wp-content/uploads/2024/05/Tab-2-EHA-SUSP4D-ARM_05.15.2024_signed.pdf",
  "If any experiments proposed in this award result in a virus with enhanced growth by more than 1 log compared to wild type strains, you must notify your NIAID Program Officer and Grants Management Specialist immediately.",
  "The 2018 award's condition ('1 log' is tenfold), quoted in HHS's 2024 suspension memo. Its existence shows NIH anticipated the work could enhance a virus.")
S("b1_oig", "b1_term", "HHS OIG",
  "The growth limit was crossed, and NIH only learned so from a late EcoHealth report it didn't chase for nearly two years.",
  "AUTHORITY", "https://oig.hhs.gov/oas/reports/region5/52100025.pdf",
  "We found that NIH was only able to conclude that research resulted in virus growth that met specified benchmarks based on a late progress report from EcoHealth that NIH failed to follow up on until nearly 2 years after its due date.",
  "The federal watchdog's January 2023 audit. It faults NIH's oversight, not the origin question.")
S("b1_susp", "b1_oig", "HHS",
  "In May 2024 HHS suspended EcoHealth Alliance and proposed it for debarment from all federal programs.",
  "AUTHORITY", "https://oversight.house.gov/wp-content/uploads/2024/05/Tab-1-EHA-SUSP4D-Notice_5.15.2024_signed.pdf",
  "I have suspended and proposed for debarment EcoHealth Alliance, Inc. (EHA) from participating in United States Federal Government procurement and nonprocurement programs.",
  "The federal government's own finding against the grantee, three years after the testimony. Its president, Peter Daszak, was suspended and proposed for debarment personally on 21 May 2024.")
C("b1_spin", "b1", "FactCheck.org",
  "The letter was no admission that NIH funded gain-of-function research by its own standard; Republicans spun it as one.",
  "AUTHORITY", "https://www.factcheck.org/2021/10/scicheck-republicans-spin-nih-letter-about-coronavirus-gain-of-function-research/",
  "Republicans were quick to interpret the letter as an admission that the agency had funded gain-of-function research.",
  "October 2021. The fact-checkers' reading: an unexpected result inside an approved study is not the thing that was denied.")
S("b1_rasmussen", "b1_spin", "Rasmussen",
  "The mouse experiment clearly wasn't gain-of-function, because nothing shows those viruses would harm humans.",
  "AUTHORITY", "https://www.factcheck.org/2021/10/scicheck-republicans-spin-nih-letter-about-coronavirus-gain-of-function-research/",
  "clearly was not gain-of-function because it's not possible to conclude the viruses would be harmful to humans",
  "Angela Rasmussen, a virologist, in the same FactCheck.org piece. The narrow test asks whether a pathogen already dangerous to humans was made more so.")
S("a1_house", "a1", "House Select Subcommittee",
  "The House investigation concluded that EcoHealth used taxpayer dollars to facilitate dangerous gain-of-function research in Wuhan.",
  "AUTHORITY", "https://oversight.house.gov/release/final-report-covid-select-concludes-2-year-investigation-issues-500-page-final-report-on-lessons-learned-and-the-path-forward/",
  "EcoHealth — under the leadership of Dr. Peter Daszak — used U.S. taxpayer dollars to facilitate dangerous gain-of-function research in Wuhan, China.",
  "The Republican majority's December 2024 final report: a finding of a party-line committee, not a court or a scientific body.")

# ---- 2. Public words versus private view on the origin (2020 - 2026) ------------
S("c1", "root", "Paul",
  "Fauci privately doubted the natural-origin account while telling the public a lab leak was implausible.",
  "EVIDENTIAL", CBS_DIARY,
  "Paul on Monday accused Fauci of saying one thing to \"Dear Diary\" while telling the public \"it's implausible that it came from the lab\"",
  "July 2026, after Fauci's diaries were released. Aimed at the public rather than Congress: not that a statement was false, but that his public confidence didn't match his private doubt.")
S("c1_diary", "c1", "Fauci",
  "In private, in January 2020, Fauci wrote that the market was not the source but the amplifier.",
  "EVIDENTIAL", "https://abcnews.com/Politics/4-takeaways-faucis-journals-ahead-senate-testimony/story?id=135171269",
  "Now we know the market was not the source, it was the amplifier. Having said that, somewhere the virus jumped from animals to humans.",
  "The private entry both sides quote. Paul reads doubt about the official story; Fauci's side reads the next sentence, a working scientist still assuming a natural jump.")
S("c1_may2020", "c1", "Fauci",
  "In public, in May 2020, Fauci said the evidence was very, very strongly leaning against the virus being artificially or deliberately manipulated.",
  "EVIDENTIAL", "https://www.nationalgeographic.com/science/article/anthony-fauci-no-scientific-evidence-the-coronavirus-was-made-in-a-chinese-lab-cvd",
  "If you look at the evolution of the virus in bats and what's out there now, [the scientific evidence] is very, very strongly leaning toward this could not have been artificially or deliberately manipulated",
  "The public statement. It speaks of deliberate manipulation, which is narrower than a lab leak of a natural virus.")
C("c1_always", "c1", "Fauci",
  "There was no hidden second view: he always said a natural origin was very highly likely, and that no one was certain.",
  "EVIDENTIAL", CBS_DIARY,
  "I had always said that the very highly likelihood is that the virus originated naturally from an animal reservoir jumping to a human. However ... I have been saying that no one is 100% certain of the origin including me and so I am calling for a thorough investigation.",
  "Fauci's answer to the diary charge, July 2026: a best estimate stated publicly, plus stated uncertainty.")
S("c1_2021", "c1_always", "Fauci",
  "In May 2021 he said publicly that the virus likely came from an animal reservoir but could have been something else.",
  "EVIDENTIAL", "https://www.deseret.com/coronavirus/2021/5/24/22451233/coronavirus-origins-dr-fauci/",
  "Certainly, the people who investigated it say it likely was the emergence from an animal reservoir that then infected individuals, but it could have been something else.",
  "On the record years before the diaries surfaced, saying in public what Paul says he only said in private.")
S("c1_paper", "c1_always", "Andersen",
  "His view rested on the science: the Proximal Origin analyses show SARS-CoV-2 is not a laboratory construct or a purposefully manipulated virus.",
  "AUTHORITY", "https://pmc.ncbi.nlm.nih.gov/articles/PMC7095063/",
  "Our analyses clearly show that SARS-CoV-2 is not a laboratory construct or a purposefully manipulated virus.",
  "Nature Medicine, March 2020, the paper Fauci's public statements leaned on. 'Not engineered' is not the same as 'did not leak from a lab'.")
S("c1_market", "c1_paper", "Worobey",
  "The earliest known cases, including those with no known link, were centred on the Huanan market.",
  "EVIDENTIAL", "https://www.research.ed.ac.uk/en/publications/the-huanan-seafood-wholesale-market-in-wuhan-was-the-early-epicen",
  "the earliest known COVID-19 cases from December 2019, including those without reported direct links, were geographically centered on this market",
  "Science, 2022: the main peer-reviewed case for a natural spillover.")
C("c1_proximal", "c1_paper", "House Select Subcommittee",
  "That paper was Fauci's own doing: he prompted it to push the preferred narrative of a natural origin.",
  "AUTHORITY", "https://oversight.house.gov/release/final-report-covid-select-concludes-2-year-investigation-issues-500-page-final-report-on-lessons-learned-and-the-path-forward/",
  "was prompted by Dr. Fauci to push the preferred narrative that COVID-19 originated in nature",
  "The December 2024 final report, making a charge the subcommittee had pressed since 2023.")
C("c1_andersen", "c1_proximal", "Andersen",
  "None of the Proximal Origin work was influenced by Fauci.",
  "EVIDENTIAL", "https://oversight.house.gov/wp-content/uploads/2023/07/Dr.-Kristian-Andersen-Written-Testimony-Corrected-Version-1.pdf",
  "None of this work was influenced by Dr. Fauci. Any suggestion that he did is untrue — as repeatedly attested by myself and other scientists involved in the paper or conference call.",
  "Kristian Andersen, the paper's lead author, in written testimony to the House, July 2023.")
C("c1_bribe", "c1_proximal", "Fauci",
  "The claim that he bribed the paper's scientists with grant money to change their minds is absolutely false.",
  "EVIDENTIAL", "https://www.foxnews.com/politics/fauci-denies-seeking-suppress-covid-19-lab-leak-origin-theory",
  "The accusation being circulated that I influenced these scientists to change their minds by bribing them with millions of dollars in grant money is absolutely false and simply preposterous.",
  "Under oath to the House, June 2024.")
C("c1_odni", "c1_always", "ODNI",
  "\"Very highly likely\" overstated it: the Energy Department and the FBI judge a lab incident the most likely start of the pandemic.",
  "AUTHORITY", "https://archive.dni.gov/files/ODNI/documents/assessments/Report-on-Potential-Links-Between-the-Wuhan-Institute-of-Virology-and-the-Origins-of-COVID-19-20230623.pdf",
  "The Department of Energy and the Federal Bureau of Investigation assess that a laboratory-associated incident was the most likely cause of the first human infection with SARS-CoV-2, although for different reasons.",
  "The declassified June 2023 report. The agencies are split: several favour natural exposure, two favour a lab incident, others can't say. All agree there's no direct evidence of an incident at the Wuhan institute.")
S("c1_bnd", "c1_odni", "German intelligence (as reported)",
  "Germany's intelligence service had judged a Wuhan lab leak 80 to 95 percent likely as early as 2020.",
  "AUTHORITY", "https://gulfnews.com/world/europe/merkel-denies-covering-up-report-on-covid-19-origins-1.500059784",
  "Germany's Federal Intelligence Service, the BND, had in 2020 judged the probability of a Wuhan lab leak being responsible for the pandemic to be between 80 and 95 per cent.",
  "Reported by German newspapers in March 2025; the assessment itself hasn't been published.")
S("c1_cia", "c1_odni", "CIA",
  "The CIA joined them in 2025: a research-related origin is more likely than a natural one, at low confidence.",
  "AUTHORITY", "https://www.cbsnews.com/news/cia-covid-likely-originated-lab-low-confidence-assessment/",
  "CIA assesses with low confidence that a research-related origin of the COVID-19 pandemic is more likely than a natural origin based on the available body of reporting.",
  "January 2025. The agency said both scenarios remain plausible.")
C("c1_sago", "c1_odni", "WHO SAGO",
  "Nothing is settled either way: all hypotheses, spillover and lab leak, must stay on the table.",
  "AUTHORITY", "https://www.who.int/news/item/27-06-2025-who-scientific-advisory-group-issues-report-on-origins-of-covid-19",
  "As things stand, all hypotheses must remain on the table, including zoonotic spillover and lab leak.",
  "The WHO's independent advisory group, June 2025: most published evidence points to a natural origin, but China hasn't shared the data needed to close the question.")

# ---- 3. The 2026 hearing and the Fifth (Jun - Aug 2026) ------------------------
S("d1", "root", "Johnson",
  "Fauci's refusal to answer the committee shows he couldn't answer truthfully without demonstrating his culpability.",
  "INFERENTIAL", "https://www.foxnews.com/politics/rand-paul-seeks-civil-contempt-charges-after-fauci-invokes-fifth-amendment-more-than-70-times",
  "I certainly understand why, on the advice of counsel, you're not answering any questions. Because I think it'd be almost impossible for you to answer the questions truthfully. Without really demonstrating what you did, your culpability.",
  "Senator Ron Johnson at the 29 July 2026 hearing: an inference from behaviour, not from a document. Fauci had declined to testify voluntarily and was subpoenaed on 23 June; at the hearing he gave an opening statement and then took the Fifth to every question.")
C("d1_valid", "d1", "Fauci's attorney",
  "Nothing follows from the silence: Fauci has a valid Fifth Amendment privilege and is on solid legal ground using it.",
  "AUTHORITY", "https://time.com/article/2026/07/30/fifth-amendment-fauci-rand-paul-covid-hearing-contempt-of-congress/",
  "We are completely confident that Dr. Fauci has a valid Fifth Amendment privilege here and is on solid legal ground in asserting it.",
  "David Schertler, 30 July 2026. If the privilege is valid, refusing to answer was lawful.")
C("d1_unsupported", "d1_valid", "Paul",
  "The privilege is unsupported, because of his blanket pardon and because he waived it by testifying at the start.",
  "INFERENTIAL", "https://www.pbs.org/newshour/politics/watch-sen-paul-says-he-will-hold-a-contempt-vote-after-fauci-invokes-5th-amendment",
  "He has cited a privilege that is unsupported because of his blanket pardon and because he waived the privilege by testifying at the start.",
  "Paul's two-part legal theory: the pardon removes the risk the Fifth protects against, and the opening statement waived whatever remained.")
S("d1_pardon", "d1_unsupported", "Biden",
  "The pardon covers any federal offences from his government service from January 2014 to January 2025.",
  "EVIDENTIAL", "https://www.presidency.ucsb.edu/documents/executive-grant-clemency-dr-anthony-s-fauci",
  "FOR ANY OFFENSES against the United States which he may have committed or taken part in during the period from January 1, 2014, through the date of this pardon arising from or in any manner related to his service",
  "The pardon warrant itself, January 2025. Federal offences only, past conduct only, tied to his official roles.")
C("d1_autopen", "d1_pardon", "Trump",
  "The Biden pardons are void because they were signed by autopen.",
  "EVIDENTIAL", "https://www.cbsnews.com/news/trump-biden-pardons-void-autopen",
  "hereby declared void, vacant, and of no further force of effect, because of the fact that they were done by Autopen.",
  "This cuts the other way from how it sounds: if the pardon were void, Fauci's exposure would be real and the Fifth plainly valid. No court has ruled on it.")
C("d1_olc", "d1_autopen", "Justice Department",
  "A president need not physically sign a document himself for it to be validly signed.",
  "AUTHORITY", "https://www.cbsnews.com/news/trump-biden-pardons-void-autopen",
  "the president need not personally perform the physical act of affixing his signature",
  "The Justice Department's 2005 analysis, as quoted by CBS.")
S("d1_brown", "d1_unsupported", "Supreme Court",
  "A pardoned witness can no longer claim the privilege, since he stands as if the offence had never been committed.",
  "AUTHORITY", "https://www.law.cornell.edu/supremecourt/text/161/591",
  "Thus, if the witness has already received a pardon, he cannot longer set up his privilege, since he stands, with respect to such offense, as if it had never been committed.",
  "Brown v. Walker, 1896: the strongest legal authority for Paul's side. The same opinion says the danger must be real and appreciable, not imaginary.")
C("d1_state", "d1_brown", "PBS News",
  "That rule doesn't reach him: the pardon is federal only, and Florida, West Virginia and Louisiana say they are subpoenaing him.",
  "EVIDENTIAL", "https://www.pbs.org/newshour/politics/why-was-fauci-held-in-contempt-for-pleading-the-5th-and-what-happens-next",
  "The pardon also doesn't shield him from state or local prosecution. Attorneys general for Florida, West Virginia and Louisiana have said they're issuing Fauci subpoenas.",
  "The Fifth only needs a real risk of prosecution somewhere, and a presidential pardon reaches only federal crimes.")
C("d1_turley", "d1_brown", "Turley",
  "The pardon only covers the past; his real risk was new: any false statement at the hearing itself.",
  "AUTHORITY", "https://www.foxnews.com/politics/can-fauci-hide-behind-fifth-game-changing-biden-pardon-may-not-provide-complete-cover-experts-say",
  "The only risk that he faced would come from any false statements that he made in the hearing",
  "Jonathan Turley, a law professor sympathetic to Paul, conceding that new testimony carries new exposure, which is what makes the privilege real.")
S("d1_lerner", "d1_unsupported", "Lankford",
  "Congress held Lois Lerner in contempt for giving an opening statement and then taking the Fifth: you can't do both.",
  "AUTHORITY", "https://www.lankford.senate.gov/news/press-releases/fauci-invokes-the-fifth-as-lankford-presses-on-closed-churches-chinas-cover-up-and-the-former-niaid-directors-own-notes/",
  "Lois Lerner came and testified, gave an opening statement, and then took the Fifth Amendment. The committee then held her in contempt for that, saying, you can't do both.",
  "The waiver half of Paul's theory, from a Republican senator who otherwise acknowledged Fauci's right to the Fifth.")
C("d1_machen", "d1_lerner", "U.S. Attorney Machen",
  "That precedent ended the other way: prosecutors found Lerner hadn't waived the privilege and refused to charge her.",
  "AUTHORITY", "https://www.govexec.com/oversight/2015/04/lerner-wont-face-contempt-charges-justice-department-says/109078/",
  "We have concluded it would not be proper to commence grand jury proceedings against a witness whose prosecution for contempt is barred by the Constitution.",
  "The Justice Department, 2015: her general denials in the opening statement did not waive the Fifth.")
S("d1_obstruct", "d1_unsupported", "Paul",
  "Obstructing a congressional investigation is against the law, and there will be repercussions.",
  "AUTHORITY", "https://www.statnews.com/2026/07/29/fauci-pleads-fifth-criticizes-rand-paul-senate-hearing/",
  "It's against the law to obstruct an investigation of Congress. There will be repercussions to your refusal to testify today",
  "Paul to Fauci at the hearing, 29 July 2026.")
S("d1_statute", "d1_obstruct", "Congress",
  "The law makes it a crime for a witness to refuse any pertinent question.",
  "AUTHORITY", "https://www.law.cornell.edu/uscode/text/2/192",
  "or who, having appeared, refuses to answer any question pertinent to the question under inquiry, shall be deemed guilty of a misdemeanor, punishable by a fine of not more than $1,000 nor less than $100",
  "The criminal contempt statute, 2 U.S.C. 192. It presumes the refusal was not privileged; a valid Fifth Amendment claim is a defence.")
S("d1_111", "d1_obstruct", "Paul",
  "He refused 111 times to answer questions pertinent to the investigation.",
  "EVIDENTIAL", "https://www.cbsnews.com/news/anthony-fauci-senate-committee-contempt-of-congress-rand-paul/",
  "The witness had many chances to answer the questions pertinent to our investigation, and he made his decision 111 times.",
  "Paul at the 6 August 2026 committee vote, which went 8 to 5 along party lines.")
C("d1_crs", "d1_111", "CRS",
  "A committee vote isn't enough: criminal contempt normally needs the full Senate to send the matter to the U.S. Attorney.",
  "AUTHORITY", "https://www.everycrsreport.com/reports/LSB10974.html",
  "the process of holding an individual in contempt generally includes a vote of the committee reporting the matter to the full House or Senate, followed by a vote of the full chamber to direct the certification of the matter to the U.S. Attorney.",
  "The Congressional Research Service on how contempt normally works. The committee sent its referral straight to the Justice Department without a floor vote.")
S("d1_stalled", "d1_crs", "Wall Street Journal (via Mediaite)",
  "And it stalled there: by late August the Justice Department still had no formal referral.",
  "EVIDENTIAL", "https://www.mediaite.com/media/print/gop-push-to-prosecute-fauci-stalls-because-even-trump-admin-officials-doubt-its-procedurally-sound-report/",
  "Paul's office did not even send over the contempt report to the vice president until late Tuesday, and the DOJ still has not received a formal referral of the matter.",
  "The last report: stuck at the certification step, with administration officials doubting the referral was procedurally sound.")

# ---- 4. Fauci's answer: a vendetta (Jan 2022 - Aug 2026) ------------------------
C("d1_sole", "root", "Fauci",
  "Paul's real aim is to make good on his pledges to put Fauci behind bars.",
  "NORMATIVE", "https://www.nbcnews.com/politics/congress/anthony-fauci-5th-amendment-testify-covid-hearing-rand-paul-rcna589793",
  "The only conclusion I can reach is that the sole reason he is calling me before this committee is to get me to say something — anything — that could vindicate his repeated public pledges that I end up, in his words — quote — 'behind bars.'",
  "Fauci's opening statement at the 29 July 2026 hearing, the one thing he said before taking the Fifth. It doesn't answer the charges; it attacks the motive behind them.")
S("d1_fundraise", "d1_sole", "Fauci",
  "Paul raises money off him: his campaign site put 'fire Dr. Fauci' next to a donate box.",
  "EVIDENTIAL", "https://www.newsweek.com/after-fauci-says-rand-paul-profiting-off-pandemic-paul-raises-funds-calling-him-hysterical-1668367",
  "Go to [the] Rand Paul website and you see 'fire Dr. Fauci' with a little box that says, 'contribute here.' You can do $5, $10, $20, $100. So, you are making a catastrophic epidemic for your political gain.",
  "Fauci to Paul at a January 2022 hearing.")
S("d1_diary_obj", "d1_sole", "Fauci",
  "Paul released Fauci's unredacted personal diary to embarrass and intimidate him.",
  "EVIDENTIAL", CBS_LIVE,
  "his publicly releasing my unredacted personal diary, aimed at embarrassing and intimidating me",
  "The same opening statement, 29 July 2026.")
C("c1_hhs", "d1_diary_obj", "HHS",
  "They weren't private diaries: HHS produced them from a government server as government records.",
  "EVIDENTIAL", "https://www.denvergazette.com/2026/07/31/how-did-faucis-diaries-make-it-public-no-one-will-say/",
  "HHS collected, searched, and produced government records from a server containing Dr. Fauci's emails and other official files in response to Senator Rand Paul's oversight request. These were government records — not private records.",
  "HHS's account of how the diaries got out, July 2026.")
S("d1_hassan", "d1_sole", "Hassan",
  "The hearing was designed to entrap him.",
  "NORMATIVE", "https://time.com/article/2026/07/30/fifth-amendment-fauci-rand-paul-covid-hearing-contempt-of-congress/",
  "It must be enormously frustrating to know that this hearing is designed to entrap you",
  "Senator Maggie Hassan to Fauci at the hearing. The argument behind it: a pardon covers the past, so the only new exposure is a misstatement made at the hearing itself.")
S("d1_predetermined", "d1_sole", "Peters",
  "The hearing capped months of biased work to legitimise a conclusion reached years ago.",
  "NORMATIVE", "https://www.yahoo.com/news/politics/articles/peters-rips-rand-paul-over-140236018.html",
  "This hearing appears to be a culmination of months of biased and selected efforts by the majority under the guise of a 'committee investigation' to basically legitimize a predetermined conclusion that was reached years ago",
  "Gary Peters, the committee's ranking Democrat, opening the hearing. He went on: Paul had called for Fauci to be fired and arrested, featured him in fundraising emails, and called for his security detail to be cancelled despite death threats.")
C("d1_website", "d1_predetermined", "Paul",
  "It was the most open investigation there is: everything is on a website, and no Democrat ever signed up for it.",
  "EVIDENTIAL", "https://www.rev.com/transcripts/fauci-congressional-hearing",
  "We actually created a website and we put all of the information on the website. It continues to operate. The reason it hasn't been accessed is no Democrat ever signed up for it. Not one Democrat.",
  "Paul's reply at the hearing, after Peters complained the minority was denied the records: 'The accusation that the committee has denied access to the minority's records is categorically false' (CBS).")
S("d1_vendetta", "d1_sole", "Fauci's attorney",
  "The contempt vote continues Paul's personal vendetta against a man who spent 50 years in public health.",
  "NORMATIVE", "https://www.nbcnews.com/politics/congress/senators-vote-holding-anthony-fauci-contempt-congress-pleading-fifth-t-rcna590985",
  "Today's resolution is a continuation of Senator Paul's personal vendetta against Dr. Fauci, who spent 50 years in public health and has saved countless lives.",
  "David Schertler, Fauci's lawyer, after the 6 August vote.")
C("d1_purpose", "d1_sole", "Paul",
  "The aim is answers on why taxpayers funded dangerous research in a Chinese lab without acceptable safety standards.",
  "EVIDENTIAL", CBS_LIVE,
  "The American people deserve to know why dangerous research was funded by NIAID in a totalitarian country in a lab that lacked acceptable safety standards.",
  "Paul's stated purpose for the 2026 hearing.")
S("lab_bsl2", "d1_purpose", "Wenstrup",
  "The lab did run below standard: EcoHealth chose biosafety level 2 as cheaper, while Ralph Baric did the same work at level 3 and urged them to.",
  "EVIDENTIAL", "https://oversight.house.gov/release/hearing-wrap-up-ecohealth-alliance-should-be-criminally-investigated-formally-debarred/",
  "you write in the proposal that the choice of BSL-2 is more 'cost effective.' Dr. Baric testified that he does this work at BSL-3 and in fact encouraged you to do the same.",
  "Brad Wenstrup, chair of the House Select Subcommittee, questioning EcoHealth's president Peter Daszak in May 2024.")
C("lab_far", "d1_purpose", "Fauci",
  "The research wasn't the danger alleged: the viruses studied were so far removed from SARS-CoV-2 they couldn't have become it.",
  "EVIDENTIAL", CBS_LIVE,
  "the viruses that were studied under the NIH grant, for the kinds of experiments that were done, molecularly were so far removed from SARS-CoV-2 that they could not have turned it into SARS-CoV-2 even if they tried",
  "Fauci at the 2026 hearing. It answers whether the grant caused the pandemic, not whether the lab was safe.")
S("lab_distant", "lab_far", "Collins",
  "NIH said the same in 2021: the bat viruses it funded are genetically far from SARS-CoV-2 and couldn't have caused the pandemic.",
  "EVIDENTIAL", "https://rollcall.com/2021/10/22/nih-grantee-in-wuhan-faces-questions-deadline-for-more-information-on-research/",
  "Analysis of published genomic data and other documents from the grantee demonstrate that the naturally occurring bat coronaviruses studied under the NIH grant are genetically far distant from SARS-CoV-2 and could not possibly have caused the COVID-19 pandemic.",
  "Francis Collins, October 2021, alongside the Tabak letter.")

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

next_id = 1
lines.append('name: "FauciCaseSourced"\n')
lines.append("# The Fauci / Paul dispute, entered by one reader in the voices of the people")
lines.append("# who said each thing. Every claim carries the page it came from and the exact")
lines.append("# words on it; the sources were opened and quoted in September 2026. Nothing is")
lines.append("# the reader's own claim, and no one has conceded anything, because no one has.")
lines.append("#")
lines.append("# Generated by generators/gen_fauci_sourced.py; edit the data there and rerun it.\n")
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
lines.append(f'  expectations {{ check_type: "USER_VIEW" claim_id: 1 username: "Paul" expected_status: "TRUE_CLAIM" }}')
lines.append(f'  expectations {{ check_type: "USER_VIEW" claim_id: {ids["a1"]} username: "Fauci" expected_status: "FALSE_CLAIM" }}')
lines.append("}")

io.open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
print(f"wrote {OUT}: {total} claims, {len(lines)} lines")
urls = {n[6] for n in N}
print(f"distinct source urls: {len(urls)}")
