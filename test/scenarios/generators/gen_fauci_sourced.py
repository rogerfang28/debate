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
    ""))

# ---- Branch A: the May 2021 testimony and the definition ----------------------
S("a1", "root", "Paul",
  "Fauci's May 2021 testimony that NIH did not fund gain-of-function research at the Wuhan Institute of Virology was false.",
  "EVIDENTIAL", "https://www.paul.senate.gov/after-silence-from-doj-dr-rand-paul-asks-u-s-attorney-to-investigate-anthony-fauci",
  "Dr. Fauci's testimony is inconsistent with facts that have since come to light.",
  "The sharpest charge. Lying to Congress under oath is a federal crime, so this says Fauci knowingly said something untrue, not that he was mistaken. Paul sent criminal referrals in 2021 and 2023; no prosecution followed.")
S("a1_words", "a1", "PolitiFact",
  "In May 2021 Fauci told the Senate that the NIH has not ever and does not now fund gain-of-function research in the Wuhan Institute of Virology.",
  "EVIDENTIAL", "https://www.politifact.com/article/2021/may/11/united-facts-america-fauci-disputes-sen-rand-paul-/",
  "has not ever and does not now fund gain-of-function research in the Wuhan Institute of Virology",
  "The sentence under oath that the whole perjury charge is built on. Nobody disputes he said it.")
S("a1_money", "a1", "FactCheck.org",
  "EcoHealth Alliance received $3.7 million over six years from the NIH and passed nearly $600,000 of it to the Wuhan Institute of Virology, with NIH's pre-approval.",
  "EVIDENTIAL", "https://www.factcheck.org/2021/05/the-wuhan-lab-and-the-gain-of-function-disagreement/",
  "EcoHealth ultimately received $3.7 million over six years from the NIH and distributed nearly $600,000 of that total to China's Wuhan Institute of Virology, a collaborator on the project, pre-approved by NIH.",
  "The money trail. Agreed by everyone; the fight is over what the money bought.")
S("a1_def", "a1", "Paul",
  "Until October 2021 the NIH website defined gain-of-function research as research that modifies a biological agent so that it confers new or enhanced activity to that agent.",
  "DEFINITIONAL", "https://www.paul.senate.gov/news-dr-paul-rep-comer-send-letter-director-nih-requesting-information-removal-gain-function-nih",
  "The update removed a section on the website titled 'Gain-of-Function Research,' which defined gain-of-function research as 'a type of research that modifies a biological agent so that it confers new or enhanced activity to that agent.'",
  "The broad, plain-language definition. NIH's own website carried it, and removed it around the day the Tabak letter went out. Under this reading the Wuhan mouse experiment counts.")
S("a1_ebright", "a1", "Ebright",
  "The grant documents make clear that Fauci's and Collins's assertions that NIH did not support gain-of-function research in Wuhan are untruthful.",
  "AUTHORITY", "https://www.newsweek.com/fauci-untruthful-congress-wuhan-lab-research-documents-show-gain-function-1627351",
  "The documents make it clear that assertions by the NIH director, Francis Collins, and the NIAID director, Anthony Fauci, that the NIH did not support gain-of-function research or potential pandemic pathogen enhancement in Wuhan are untruthful.",
  "Richard Ebright is a Rutgers molecular biologist and the most-cited scientific critic. He says the work fails even the narrow regulatory test, not just the broad one.")
S("a1_house", "a1", "House Select Subcommittee",
  "EcoHealth used U.S. taxpayer dollars to facilitate dangerous gain-of-function research in Wuhan, China.",
  "AUTHORITY", "https://oversight.house.gov/release/final-report-covid-select-concludes-2-year-investigation-issues-500-page-final-report-on-lessons-learned-and-the-path-forward/",
  "EcoHealth — under the leadership of Dr. Peter Daszak — used U.S. taxpayer dollars to facilitate dangerous gain-of-function research in Wuhan, China.",
  "The Republican majority's December 2024 final report. A finding of a party-line committee, not a court or a scientific body.")

C("a1_p3co", "a1", "Fauci",
  "Under the regulatory and operative P3CO definition, the NIH did not fund gain-of-function research at the Wuhan Institute of Virology.",
  "DEFINITIONAL", "https://www.govinfo.gov/content/pkg/CHRG-118hhrg55830/html/CHRG-118hhrg55830.htm",
  "According to the regulatory and operative definition of P3CO, the NIH did not fund gain-of-function research at the Wuhan Institute of Virology.",
  "The heart of the defence. Fauci disputes no fact, only which dictionary applies: the rules governing federal funding use a narrow technical term, and the bat-virus work did not meet it.")
S("a1_pause", "a1_p3co", "NIH",
  "The 2014 federal funding pause covered projects reasonably anticipated to give influenza, MERS or SARS viruses enhanced pathogenicity or transmissibility, and was lifted in December 2017 under the P3CO framework.",
  "AUTHORITY", "https://grants.nih.gov/grants/guide/notice-files/NOT-OD-17-071.html",
  "instituting a funding pause on gain-of-function research projects that could be reasonably anticipated to confer attributes to influenza, MERS, or SARS viruses such that the resulting virus has enhanced pathogenicity and/or transmissibility",
  "Where the narrow definition comes from: a three-year pause on certain risky work, replaced in 2017 by extra review for a specific category of pathogen.")
S("a1_collins", "a1_p3co", "Collins",
  "NIH would not have supported research on coronaviruses that might produce a virus more dangerous than what nature has already come up with.",
  "AUTHORITY", "https://hughhewitt.com/nih-director-francis-collins-on-gain-of-function-research-at-nih-and-wuhan-lab",
  "NIH would not have supported any such research on coronaviruses, because there are risks there that you might actually end up producing a virus that has a higher danger attached to it than what nature has already come up with.",
  "The NIH director, two days after the testimony, backing Fauci and framing gain-of-function as making a virus more dangerous than nature's version.")
S("a1_rasmussen", "a1_p3co", "Rasmussen",
  "The Wuhan mouse experiment clearly was not gain-of-function because it is not possible to conclude the viruses would be harmful to humans.",
  "AUTHORITY", "https://www.factcheck.org/2021/10/scicheck-republicans-spin-nih-letter-about-coronavirus-gain-of-function-research/",
  "clearly was not gain-of-function because it's not possible to conclude the viruses would be harmful to humans",
  "Angela Rasmussen is a virologist quoted by FactCheck.org. The narrow test asks whether the pathogen was already dangerous to humans and made more so.")
C("a1_nebulous", "a1_p3co", "Paul",
  "Fauci himself called gain-of-function a very nebulous term, so his denial rested on a definition he admits is unsettled.",
  "DEFINITIONAL", "https://www.foxnews.com/politics/rand-paul-anthony-fauci-senate-hearing-gain-of-function.print",
  "Gain of function is a very nebulous term that outside parties have worked on assigning a more precise definition to.",
  "Both sides quote this line against each other. Paul reads it as an admission the denial was slippery; Fauci's side reads it as the reason a precise regulatory definition is the only fair test.")
C("a1_never_lied", "a1", "Fauci",
  "Fauci has never lied before Congress and does not retract the May 2021 statement.",
  "EVIDENTIAL", "https://www.factcheck.org/2021/07/scicheck-fauci-and-paul-round-2/",
  "Sen. Paul, I have never lied before the Congress, and I do not retract that statement.",
  "The July 2021 denial to Paul's face, in the exchange where Fauci also told Paul he did not know what he was talking about.")

# ---- Branch B: the Tabak letter ------------------------------------------------
S("b1", "a1", "Ebright",
  "The October 2021 NIH letter shows that Collins and Fauci lied to Congress, to the press and to the public.",
  "AUTHORITY", "https://www.washingtonexaminer.com/news/801375/bombshell-nih-letter-on-bat-coronavirus-research-reveals-faucis-big-lie-professor-says/",
  "Collins and Fauci lied to Congress, lied to the press, and lied to the public.",
  "A second, independent route to the perjury charge that does not depend on the definition fight: NIH's own letter describing what the funded work did.")
S("b1_tabak", "b1", "Tabak",
  "In a limited experiment, mice infected with the SHC014 WIV1 bat coronavirus became sicker than those infected with the WIV1 bat coronavirus.",
  "EVIDENTIAL", "https://www.documentcloud.org/documents/21674679-tabak-letter-to-comer-oct-20-2021",
  "In this limited experiment, laboratory mice infected with the SHC014 WIV1 bat coronavirus became sicker than those infected with the WIV1 bat coronavirus.",
  "The most quoted document in the whole affair. Lawrence Tabak was NIH's principal deputy director; the letter went to Rep. Comer on October 20, 2021. Nobody disputes a word of it; both sides say it vindicates them.")
S("b1_term", "b1", "HHS",
  "The 2018 award required EcoHealth to notify NIAID immediately if any experiment produced a virus with enhanced growth of more than 1 log over the wild type.",
  "EVIDENTIAL", "https://oversight.house.gov/wp-content/uploads/2024/05/Tab-2-EHA-SUSP4D-ARM_05.15.2024_signed.pdf",
  "If any experiments proposed in this award result in a virus with enhanced growth by more than 1 log compared to wild type strains, you must notify your NIAID Program Officer and Grants Management Specialist immediately.",
  "The grant condition EcoHealth is said to have broken. Its existence shows NIH anticipated that the work could enhance a virus.")
S("b1_oig", "b1", "HHS OIG",
  "NIH could only conclude the research met its growth benchmark from a late EcoHealth progress report that NIH failed to follow up on for nearly two years.",
  "AUTHORITY", "https://oig.hhs.gov/oas/reports/region5/52100025.pdf",
  "We found that NIH was only able to conclude that research resulted in virus growth that met specified benchmarks based on a late progress report from EcoHealth that NIH failed to follow up on until nearly 2 years after its due date.",
  "The federal watchdog's January 2023 audit. It faults NIH's oversight, not the origin question.")
C("b1_distant", "b1", "Collins",
  "The bat coronaviruses studied under the NIH grant are genetically far distant from SARS-CoV-2 and could not possibly have caused the COVID-19 pandemic.",
  "EVIDENTIAL", "https://rollcall.com/2021/10/22/nih-grantee-in-wuhan-faces-questions-deadline-for-more-information-on-research/",
  "Analysis of published genomic data and other documents from the grantee demonstrate that the naturally occurring bat coronaviruses studied under the NIH grant are genetically far distant from SARS-CoV-2 and could not possibly have caused the COVID-19 pandemic.",
  "Note what this answers and what it does not: it says the funded viruses did not become the pandemic virus. It does not say the mice did not get sicker.")
C("b1_spin", "b1", "FactCheck.org",
  "The Tabak letter was not an admission that NIH funded gain-of-function research under its own standard, and Republicans spun it as one.",
  "AUTHORITY", "https://www.factcheck.org/2021/10/scicheck-republicans-spin-nih-letter-about-coronavirus-gain-of-function-research/",
  "Republicans were quick to interpret the letter as an admission that the agency had funded gain-of-function research.",
  "The fact-checkers' reading: an unexpected result inside an approved study is not the thing that was denied.")

# ---- Branch B2: the lab, its safety and its grantee ------------------------------
S("lab", "root", "Paul",
  "NIAID funded research in a lab in China that lacked acceptable safety standards.",
  "EVIDENTIAL", CBS_LIVE,
  "why dangerous research was funded by NIAID in a totalitarian country in a lab that lacked acceptable safety standards",
  "Paul's stated purpose for the 2026 hearing. Offered here for the root because Paul's case is that the risk was known and denied.")
S("lab_bsl2", "lab", "Wenstrup",
  "EcoHealth's proposal chose biosafety level 2 as more cost effective, while Ralph Baric did the same work at BSL-3 and urged EcoHealth to do likewise.",
  "EVIDENTIAL", "https://oversight.house.gov/release/hearing-wrap-up-ecohealth-alliance-should-be-criminally-investigated-formally-debarred/",
  "you write in the proposal that the choice of BSL-2 is more 'cost effective.' Dr. Baric testified that he does this work at BSL-3 and in fact encouraged you to do the same.",
  "Brad Wenstrup chaired the House Select Subcommittee; this is from his questioning of EcoHealth's president Peter Daszak in May 2024.")
S("lab_defuse", "lab", "The Intercept",
  "In 2018 EcoHealth Alliance submitted a grant proposal to DARPA, later rejected, describing planned work on bat coronaviruses.",
  "EVIDENTIAL", "https://theintercept.com/2021/09/23/coronavirus-research-grant-darpa/",
  "A grant proposal written by the U.S.-based nonprofit the EcoHealth Alliance and submitted in 2018 to the Defense Advanced Research Projects Agency, or DARPA, provides evidence that the group was working",
  "The DEFUSE proposal. DARPA did not fund it; critics point to what it planned, defenders to the fact it was never funded.")
S("lab_susp", "lab", "HHS",
  "In May 2024 HHS suspended EcoHealth Alliance and proposed it for debarment from all federal procurement and nonprocurement programs.",
  "AUTHORITY", "https://oversight.house.gov/wp-content/uploads/2024/05/Tab-1-EHA-SUSP4D-Notice_5.15.2024_signed.pdf",
  "I have suspended and proposed for debarment EcoHealth Alliance, Inc. (EHA) from participating in United States Federal Government procurement and nonprocurement programs.",
  "The federal government's own finding against the grantee, three years after the testimony.")
S("lab_daszak", "lab", "HHS",
  "In May 2024 HHS also suspended Peter Daszak personally and proposed him for debarment as EcoHealth's president and principal investigator.",
  "AUTHORITY", "https://oversight.house.gov/wp-content/uploads/2024/05/HHS-SUSP4D-Notice-of-Dr.-Peter-Daszak_05.21.2024_Redacted.pdf",
  "I have suspended you and proposed you for debarment, related to your respective roles as the President of EcoHealth Alliance, Inc. (EHA) and as Program Director (PD)/Principal Investigator (PI)")
S("lab_reinstated", "lab", "NIH",
  "In July 2020 NIH withdrew its April termination of the EcoHealth grant and reinstated it, while suspending its activities pending biosafety answers.",
  "EVIDENTIAL", "https://oversight.house.gov/wp-content/uploads/2021/05/Eco-Health-Lab-letter-July-8.pdf",
  "has withdrawn its termination of grant R01AI110964, which supports the project Understanding the Risk of Bat Coronavirus Emergence. Accordingly, the grant is reinstated.",
  "The grant was killed under political pressure in April 2020, revived on paper in July, and frozen in practice. Michael Lauer signed for NIH.")
C("lab_far", "lab", "Fauci",
  "The viruses studied under the NIH grant were molecularly so far removed from SARS-CoV-2 that they could not have turned into it.",
  "EVIDENTIAL", CBS_LIVE,
  "the viruses that were studied under the NIH grant, for the kinds of experiments that were done, molecularly were so far removed from SARS-CoV-2 that they could not have turned it into SARS-CoV-2 even if they tried",
  "Fauci's standing answer whenever the lab comes up. It answers whether the grant caused the pandemic, not whether the lab was safe.")

# ---- Branch C: public words versus private view on the origin -----------------
S("c1", "root", "Paul",
  "Fauci privately doubted the natural-origin account while telling the public a lab leak was implausible.",
  "EVIDENTIAL", CBS_DIARY,
  "Paul on Monday accused Fauci of saying one thing to \"Dear Diary\" while telling the public \"it's implausible that it came from the lab\"",
  "A separate accusation, aimed at the public rather than Congress: not that a statement was false, but that his public confidence did not match his private doubt. It rests on the diaries released in July 2026.")
S("c1_may2020", "c1", "Fauci",
  "In May 2020 Fauci said the scientific evidence was very, very strongly leaning toward the virus not having been artificially or deliberately manipulated.",
  "EVIDENTIAL", "https://www.nationalgeographic.com/science/article/anthony-fauci-no-scientific-evidence-the-coronavirus-was-made-in-a-chinese-lab-cvd",
  "If you look at the evolution of the virus in bats and what's out there now, [the scientific evidence] is very, very strongly leaning toward this could not have been artificially or deliberately manipulated",
  "The public statement. Note it speaks of deliberate manipulation, which is narrower than a lab leak of a natural virus.")
S("c1_diary", "c1", "Fauci",
  "In a January 2020 diary entry Fauci wrote that the market was not the source but the amplifier, while assuming the virus jumped from animals to humans.",
  "EVIDENTIAL", "https://abcnews.com/Politics/4-takeaways-faucis-journals-ahead-senate-testimony/story?id=135171269",
  "Now we know the market was not the source, it was the amplifier. Having said that, somewhere the virus jumped from animals to humans.",
  "The private entry both sides quote. Paul reads doubt about the official story; Fauci's side reads a working scientist still assuming a natural jump.")
S("c1_hhs", "c1_diary", "HHS",
  "HHS collected, searched and produced records from a government server holding Fauci's emails and files in response to Paul's oversight request, treating them as government records.",
  "EVIDENTIAL", "https://www.denvergazette.com/2026/07/31/how-did-faucis-diaries-make-it-public-no-one-will-say/",
  "HHS collected, searched, and produced government records from a server containing Dr. Fauci's emails and other official files in response to Senator Rand Paul's oversight request. These were government records — not private records.",
  "How the diaries got out, in HHS's account. Fauci calls them his personal diary; HHS calls them government records.")
S("c1_morens", "c1", "Morens",
  "Fauci's senior adviser David Morens wrote that he had learned from the FOIA office how to make emails disappear after being FOIA'd but before the search starts.",
  "EVIDENTIAL", "https://www.thenation.com/article/archive/nih-foia-covid-origins-morens-hearing/",
  "i learned from our foia lady here how to make emails disappear after i am foia'd but before the search starts, so i think we are all safe.",
  "An email in Morens's own words, offered by Paul's side as evidence of a culture of concealment around Fauci's office. Morens, not Fauci, wrote it.")
S("c1_proximal", "c1", "House Select Subcommittee",
  "The Proximal Origin paper was prompted by Fauci to push the preferred narrative that COVID-19 originated in nature.",
  "AUTHORITY", "https://oversight.house.gov/release/final-report-covid-select-concludes-2-year-investigation-issues-500-page-final-report-on-lessons-learned-and-the-path-forward/",
  "was prompted by Dr. Fauci to push the preferred narrative that COVID-19 originated in nature",
  "Proximal Origin is the March 2020 Nature Medicine paper that told the world the virus was not engineered. The charge is that Fauci arranged it.")
C("c1_andersen", "c1_proximal", "Andersen",
  "None of the Proximal Origin work was influenced by Fauci, and any suggestion that it was is untrue.",
  "EVIDENTIAL", "https://oversight.house.gov/wp-content/uploads/2023/07/Dr.-Kristian-Andersen-Written-Testimony-Corrected-Version-1.pdf",
  "None of this work was influenced by Dr. Fauci. Any suggestion that he did is untrue — as repeatedly attested by myself and other scientists involved in the paper or conference call.",
  "Kristian Andersen was the paper's lead author, testifying in writing to the House in July 2023.")
C("c1_bribe", "c1_proximal", "Fauci",
  "The accusation that Fauci influenced the Proximal Origin scientists by bribing them with grant money is absolutely false.",
  "EVIDENTIAL", "https://www.foxnews.com/politics/fauci-denies-seeking-suppress-covid-19-lab-leak-origin-theory",
  "The accusation being circulated that I influenced these scientists to change their minds by bribing them with millions of dollars in grant money is absolutely false and simply preposterous.",
  "Fauci under oath to the House, June 2024.")
C("c1_always", "c1", "Fauci",
  "Fauci always said natural origin was very highly likely, and once a lab leak was being discussed he said no one was certain and called for a thorough investigation.",
  "EVIDENTIAL", CBS_DIARY,
  "I had always said that the very highly likelihood is that the virus originated naturally from an animal reservoir jumping to a human. However ... I have been saying that no one is 100% certain of the origin including me and so I am calling for a thorough investigation.",
  "Fauci's answer to the diary charge: a best estimate stated publicly, plus stated uncertainty, is not a hidden second position.")
S("c1_2021", "c1_always", "Fauci",
  "In May 2021 Fauci said publicly that the virus likely emerged from an animal reservoir but could have been something else.",
  "EVIDENTIAL", "https://www.deseret.com/coronavirus/2021/5/24/22451233/coronavirus-origins-dr-fauci/",
  "Certainly, the people who investigated it say it likely was the emergence from an animal reservoir that then infected individuals, but it could have been something else.",
  "On the record a year before the diaries surfaced, saying in public what Paul says he only said in private.")
S("c1_paper", "c1_always", "Andersen",
  "The Proximal Origin analyses show that SARS-CoV-2 is not a laboratory construct or a purposefully manipulated virus.",
  "AUTHORITY", "https://pmc.ncbi.nlm.nih.gov/articles/PMC7095063/",
  "Our analyses clearly show that SARS-CoV-2 is not a laboratory construct or a purposefully manipulated virus.",
  "The scientific basis Fauci's public statements leaned on in 2020. Note that 'not engineered' is not the same as 'did not leak from a lab'.")
S("c1_market", "c1_paper", "Worobey",
  "The earliest known COVID-19 cases from December 2019, including those without reported links, were geographically centered on the Huanan market.",
  "EVIDENTIAL", "https://www.research.ed.ac.uk/en/publications/the-huanan-seafood-wholesale-market-in-wuhan-was-the-early-epicen",
  "the earliest known COVID-19 cases from December 2019, including those without reported direct links, were geographically centered on this market",
  "The main peer-reviewed case for a natural spillover, published in Science in 2022.")
C("c1_odni", "c1_paper", "ODNI",
  "The Department of Energy and the FBI assess that a laboratory-associated incident was the most likely cause of the first human infection with SARS-CoV-2.",
  "AUTHORITY", "https://archive.dni.gov/files/ODNI/documents/assessments/Report-on-Potential-Links-Between-the-Wuhan-Institute-of-Virology-and-the-Origins-of-COVID-19-20230623.pdf",
  "The Department of Energy and the Federal Bureau of Investigation assess that a laboratory-associated incident was the most likely cause of the first human infection with SARS-CoV-2, although for different reasons.",
  "The declassified 2023 report. The agencies are split: several favour natural exposure, two favour a lab incident, others cannot say. All agree there is no direct evidence of an incident at the Wuhan institute.")
C("c1_cia", "c1_paper", "CIA",
  "The CIA assesses with low confidence that a research-related origin of the pandemic is more likely than a natural origin.",
  "AUTHORITY", "https://www.cbsnews.com/news/cia-covid-likely-originated-lab-low-confidence-assessment/",
  "CIA assesses with low confidence that a research-related origin of the COVID-19 pandemic is more likely than a natural origin based on the available body of reporting.",
  "January 2025. Low confidence, and the agency said both scenarios remain plausible.")
C("c1_bnd", "c1_paper", "German intelligence (as reported)",
  "Germany's Federal Intelligence Service judged in 2020 that the probability of a Wuhan lab leak causing the pandemic was between 80 and 95 percent.",
  "AUTHORITY", "https://gulfnews.com/world/europe/merkel-denies-covering-up-report-on-covid-19-origins-1.500059784",
  "Germany's Federal Intelligence Service, the BND, had in 2020 judged the probability of a Wuhan lab leak being responsible for the pandemic to be between 80 and 95 per cent.",
  "Reported by German newspapers in March 2025; the assessment itself has not been published.")
C("c1_sago", "c1_paper", "WHO SAGO",
  "All hypotheses on the origin must remain on the table, including zoonotic spillover and a lab leak.",
  "AUTHORITY", "https://www.who.int/news/item/27-06-2025-who-scientific-advisory-group-issues-report-on-origins-of-covid-19",
  "As things stand, all hypotheses must remain on the table, including zoonotic spillover and lab leak.",
  "The WHO's independent advisory group, June 2025: most published evidence points to a natural origin, but China has not shared the data needed to close the question.")

# ---- Branch D: the 2026 hearing, the pardon and the Fifth ---------------------
S("d1", "root", "Johnson",
  "Fauci's refusal to answer the committee's questions indicates he could not answer them truthfully without demonstrating his culpability.",
  "INFERENTIAL", "https://www.foxnews.com/politics/rand-paul-seeks-civil-contempt-charges-after-fauci-invokes-fifth-amendment-more-than-70-times",
  "I certainly understand why, on the advice of counsel, you're not answering any questions. Because I think it'd be almost impossible for you to answer the questions truthfully. Without really demonstrating what you did, your culpability.",
  "The newest layer, and nothing to do with virology. Senator Ron Johnson's inference from Fauci's silence at the July 29, 2026 hearing. An inference from behaviour, not from a document.")
S("d1_subpoena", "d1", "Paul",
  "Fauci told the committee he would not voluntarily testify after previously agreeing to, and was subpoenaed on June 23, 2026.",
  "EVIDENTIAL", "https://www.upi.com/Top_News/US/2026/06/23/fauci-subpoena-rand-paul/3151782227311/",
  "Last week, Anthony Fauci notified us that he will not voluntarily testify before the Senate Homeland Security and Governmental Affairs Committee, even though he had previously agreed to do so. Therefore, today we have issued a subpoena for him to publicly testify.")
S("d1_111", "d1", "Paul",
  "Fauci had many chances to answer questions pertinent to the investigation and declined 111 times.",
  "EVIDENTIAL", "https://www.cbsnews.com/news/anthony-fauci-senate-committee-contempt-of-congress-rand-paul/",
  "The witness had many chances to answer the questions pertinent to our investigation, and he made his decision 111 times.",
  "Paul at the August 6, 2026 committee vote, which went 8 to 5 along party lines to hold Fauci in contempt.")
C("d1_sole", "d1", "Fauci",
  "The sole reason Paul called Fauci before the committee was to get him to say something that could vindicate Paul's repeated public pledges that Fauci would end up behind bars.",
  "NORMATIVE", "https://www.nbcnews.com/politics/congress/anthony-fauci-5th-amendment-testify-covid-hearing-rand-paul-rcna589793",
  "The only conclusion I can reach is that the sole reason he is calling me before this committee is to get me to say something — anything — that could vindicate his repeated public pledges that I end up, in his words — quote — 'behind bars.'",
  "Fauci's opening statement, the one thing he said before taking the Fifth. It does not defend the silence; it attacks the motive for the hearing.")
S("d1_fundraise", "d1_sole", "Fauci",
  "Paul's campaign website said 'fire Dr. Fauci' next to a box inviting contributions.",
  "EVIDENTIAL", "https://www.newsweek.com/after-fauci-says-rand-paul-profiting-off-pandemic-paul-raises-funds-calling-him-hysterical-1668367",
  "Go to [the] Rand Paul website and you see 'fire Dr. Fauci' with a little box that says, 'contribute here.' You can do $5, $10, $20, $100. So, you are making a catastrophic epidemic for your political gain.",
  "Fauci to Paul at a January 2022 hearing.")
S("d1_diary_obj", "d1_sole", "Fauci",
  "Paul publicly released Fauci's unredacted personal diary, aimed at embarrassing and intimidating him.",
  "EVIDENTIAL", CBS_LIVE,
  "his publicly releasing my unredacted personal diary, aimed at embarrassing and intimidating me")
S("d1_hassan", "d1_sole", "Hassan",
  "The hearing was designed to entrap Fauci.",
  "NORMATIVE", "https://time.com/article/2026/07/30/fifth-amendment-fauci-rand-paul-covid-hearing-contempt-of-congress/",
  "It must be enormously frustrating to know that this hearing is designed to entrap you",
  "Senator Maggie Hassan to Fauci at the hearing. The argument behind it: a pardon covers the past, so the only new exposure is a misstatement made at the hearing itself.")
S("d1_vendetta", "d1_sole", "Fauci's attorney",
  "The contempt resolution is a continuation of Paul's personal vendetta against Fauci, who spent 50 years in public health.",
  "NORMATIVE", "https://www.nbcnews.com/politics/congress/senators-vote-holding-anthony-fauci-contempt-congress-pleading-fifth-t-rcna590985",
  "Today's resolution is a continuation of Senator Paul's personal vendetta against Dr. Fauci, who spent 50 years in public health and has saved countless lives.",
  "David Schertler, Fauci's lawyer, after the August 6 vote.")
C("d1_purpose", "d1_sole", "Paul",
  "The hearing's purpose was to get answers and an apology for why U.S. taxpayers funded dangerous research at a lab in China without adequate safety standards.",
  "EVIDENTIAL", CBS_LIVE,
  "The American people deserve to know why dangerous research was funded by NIAID in a totalitarian country in a lab that lacked acceptable safety standards.")
C("d1_valid", "d1", "Fauci's attorney",
  "Fauci has a valid Fifth Amendment privilege and is on solid legal ground in asserting it.",
  "AUTHORITY", "https://time.com/article/2026/07/30/fifth-amendment-fauci-rand-paul-covid-hearing-contempt-of-congress/",
  "We are completely confident that Dr. Fauci has a valid Fifth Amendment privilege here and is on solid legal ground in asserting it.",
  "The legal defence of the silence itself. If the privilege is valid, refusing to answer was lawful and nothing follows from it.")
S("d1_state", "d1_valid", "PBS News",
  "The pardon does not shield Fauci from state or local prosecution, and the attorneys general of Florida, West Virginia and Louisiana have said they are issuing him subpoenas.",
  "EVIDENTIAL", "https://www.pbs.org/newshour/politics/why-was-fauci-held-in-contempt-for-pleading-the-5th-and-what-happens-next",
  "The pardon also doesn't shield him from state or local prosecution. Attorneys general for Florida, West Virginia and Louisiana have said they're issuing Fauci subpoenas.",
  "Why the privilege can survive a federal pardon: the Fifth only needs a real risk of prosecution somewhere, and a presidential pardon reaches only federal crimes.")
S("d1_turley", "d1_valid", "Turley",
  "The only risk Fauci faced at the hearing would come from any false statements he made in the hearing itself.",
  "AUTHORITY", "https://www.foxnews.com/politics/can-fauci-hide-behind-fifth-game-changing-biden-pardon-may-not-provide-complete-cover-experts-say",
  "The only risk that he faced would come from any false statements that he made in the hearing",
  "Jonathan Turley, a law professor sympathetic to Paul, conceding that new testimony carries new exposure. Offered here because that concession is exactly what makes the privilege real.")
C("d1_unsupported", "d1_valid", "Paul",
  "Fauci's privilege is unsupported because of his blanket pardon and because he waived it by testifying at the start.",
  "INFERENTIAL", "https://www.pbs.org/newshour/politics/watch-sen-paul-says-he-will-hold-a-contempt-vote-after-fauci-invokes-5th-amendment",
  "He has cited a privilege that is unsupported because of his blanket pardon and because he waived the privilege by testifying at the start.",
  "Paul's two-part legal theory: the pardon removes the risk the Fifth protects against, and the opening statement waived whatever remained.")
S("d1_pardon", "d1_unsupported", "Biden",
  "The pardon covers any federal offenses Fauci may have committed from January 1, 2014 through January 19, 2025 arising from his government service.",
  "EVIDENTIAL", "https://www.presidency.ucsb.edu/documents/executive-grant-clemency-dr-anthony-s-fauci",
  "FOR ANY OFFENSES against the United States which he may have committed or taken part in during the period from January 1, 2014, through the date of this pardon arising from or in any manner related to his service",
  "The pardon warrant itself. Federal offenses only, past conduct only, tied to his official roles.")
S("d1_brown", "d1_unsupported", "Supreme Court",
  "If a witness has already received a pardon, he can no longer set up his privilege, since he stands as if the offense had never been committed.",
  "AUTHORITY", "https://www.law.cornell.edu/supremecourt/text/161/591",
  "Thus, if the witness has already received a pardon, he cannot longer set up his privilege, since he stands, with respect to such offense, as if it had never been committed.",
  "Brown v. Walker, 1896. The strongest legal authority for Paul's side. The same opinion says the danger must be real and appreciable, not imaginary.")
S("d1_lerner", "d1_unsupported", "Lankford",
  "Congress held Lois Lerner in contempt after she gave an opening statement and then took the Fifth, on the ground that you cannot do both.",
  "AUTHORITY", "https://www.lankford.senate.gov/news/press-releases/fauci-invokes-the-fifth-as-lankford-presses-on-closed-churches-chinas-cover-up-and-the-former-niaid-directors-own-notes/",
  "Lois Lerner came and testified, gave an opening statement, and then took the Fifth Amendment. The committee then held her in contempt for that, saying, you can't do both.",
  "The waiver half of Paul's theory, from a Republican senator who otherwise acknowledged Fauci's right to invoke the Fifth.")
C("d1_machen", "d1_lerner", "U.S. Attorney Machen",
  "It would not be proper to begin grand jury proceedings against a witness whose prosecution for contempt is barred by the Constitution, because Lerner's general denials did not waive her privilege.",
  "AUTHORITY", "https://www.govexec.com/oversight/2015/04/lerner-wont-face-contempt-charges-justice-department-says/109078/",
  "We have concluded it would not be proper to commence grand jury proceedings against a witness whose prosecution for contempt is barred by the Constitution.",
  "How the Lerner precedent actually ended: the Justice Department refused to prosecute in 2015, finding her opening statement did not waive the Fifth.")
C("d1_autopen", "d1_pardon", "Trump",
  "The Biden pardons are void because they were signed by autopen.",
  "EVIDENTIAL", "https://www.cbsnews.com/news/trump-biden-pardons-void-autopen",
  "hereby declared void, vacant, and of no further force of effect, because of the fact that they were done by Autopen.",
  "Cuts the other way from how it sounds: if the pardon were void, Fauci's exposure would be real and the Fifth plainly valid. No court has ruled on it.")
C("d1_olc", "d1_autopen", "Justice Department",
  "The president need not personally perform the physical act of affixing his signature for a document to be validly signed.",
  "AUTHORITY", "https://www.cbsnews.com/news/trump-biden-pardons-void-autopen",
  "the president need not personally perform the physical act of affixing his signature",
  "The Justice Department's 2005 analysis, as quoted by CBS.")
C("d1_predetermined", "d1", "Peters",
  "The hearing was the culmination of months of biased efforts by the majority to legitimize a predetermined conclusion reached years ago.",
  "NORMATIVE", "https://www.yahoo.com/news/politics/articles/peters-rips-rand-paul-over-140236018.html",
  "This hearing appears to be a culmination of months of biased and selected efforts by the majority under the guise of a 'committee investigation' to basically legitimize a predetermined conclusion that was reached years ago",
  "Gary Peters is the committee's ranking Democrat.")
S("d1_records", "d1_predetermined", "Peters",
  "The committee denied the minority access to the investigation's records.",
  "EVIDENTIAL", CBS_LIVE,
  "The accusation that the committee has denied access to the minority's records is categorically false")
C("d1_website", "d1_records", "Paul",
  "The committee put all of the investigation's information on a website that still operates, and no Democrat ever signed up for it.",
  "EVIDENTIAL", "https://www.rev.com/transcripts/fauci-congressional-hearing",
  "We actually created a website and we put all of the information on the website. It continues to operate. The reason it hasn't been accessed is no Democrat ever signed up for it. Not one Democrat.")
S("d1_obstruct", "d1", "Paul",
  "It is against the law to obstruct an investigation of Congress, and there will be repercussions for Fauci's refusal to testify.",
  "AUTHORITY", "https://www.statnews.com/2026/07/29/fauci-pleads-fifth-criticizes-rand-paul-senate-hearing/",
  "It's against the law to obstruct an investigation of Congress. There will be repercussions to your refusal to testify today")
S("d1_statute", "d1_obstruct", "Congress",
  "A witness who appears but refuses to answer any question pertinent to the inquiry is guilty of a misdemeanor punishable by a fine and imprisonment.",
  "AUTHORITY", "https://www.law.cornell.edu/uscode/text/2/192",
  "or who, having appeared, refuses to answer any question pertinent to the question under inquiry, shall be deemed guilty of a misdemeanor, punishable by a fine of not more than $1,000 nor less than $100",
  "The criminal contempt statute, 2 U.S.C. 192. It presumes the refusal was not privileged; a valid Fifth Amendment claim is a defence.")
C("d1_crs", "d1_obstruct", "CRS",
  "Criminal contempt generally requires a vote of the full House or Senate to certify the matter to the U.S. Attorney, not just a committee vote.",
  "AUTHORITY", "https://www.everycrsreport.com/reports/LSB10974.html",
  "the process of holding an individual in contempt generally includes a vote of the committee reporting the matter to the full House or Senate, followed by a vote of the full chamber to direct the certification of the matter to the U.S. Attorney.",
  "The Congressional Research Service on how contempt normally works. The committee sent its referral straight to the Justice Department without a floor vote.")
S("d1_stalled", "d1_crs", "Wall Street Journal (via Mediaite)",
  "By late August 2026 the Justice Department had still not received a formal referral, and Paul's office had only just sent the contempt report to the vice president.",
  "EVIDENTIAL", "https://www.mediaite.com/media/print/gop-push-to-prosecute-fauci-stalls-because-even-trump-admin-officials-doubt-its-procedurally-sound-report/",
  "Paul's office did not even send over the contempt report to the vice president until late Tuesday, and the DOJ still has not received a formal referral of the matter.",
  "Where things stood at the last report: stalled at the certification step, with administration officials doubting the referral was procedurally sound.")

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
