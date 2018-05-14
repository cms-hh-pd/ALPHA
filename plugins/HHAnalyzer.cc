// -*- C++ -*-
// Package:    Analysis/HHAnalyzer
// Class:      HHAnalyzer

#include "HHAnalyzer.h"

HHAnalyzer::HHAnalyzer(const edm::ParameterSet& iConfig):
    GenPSet(iConfig.getParameter<edm::ParameterSet>("genSet")),
    PileupPSet(iConfig.getParameter<edm::ParameterSet>("pileupSet")),
    TriggerPSet(iConfig.getParameter<edm::ParameterSet>("triggerSet")),
    ElectronPSet(iConfig.getParameter<edm::ParameterSet>("electronSet")),
    MuonPSet(iConfig.getParameter<edm::ParameterSet>("muonSet")),
    JetPSet(iConfig.getParameter<edm::ParameterSet>("jetSet")),
    FatJetPSet(iConfig.getParameter<edm::ParameterSet>("fatJetSet")),
    WriteNJets(iConfig.getParameter<int>("writeNJets")),
    WriteNFatJets(iConfig.getParameter<int>("writeNFatJets")),
    HistFile(iConfig.getParameter<std::string>("histFile")),
    Verbose(iConfig.getParameter<bool>("verbose"))
{
    //now do what ever initialization is needed
    usesResource("TFileService");
    
    // Initialize Objects
    theGenAnalyzer      = new GenAnalyzer(GenPSet, consumesCollector());
    thePileupAnalyzer   = new PileupAnalyzer(PileupPSet, consumesCollector());
    theTriggerAnalyzer  = new TriggerAnalyzer(TriggerPSet, consumesCollector());
    theElectronAnalyzer = new ElectronAnalyzer(ElectronPSet, consumesCollector());
    theMuonAnalyzer     = new MuonAnalyzer(MuonPSet, consumesCollector());
    theJetAnalyzer      = new JetAnalyzer(JetPSet, consumesCollector());    

    std::vector<std::string> TriggerList(TriggerPSet.getParameter<std::vector<std::string> >("paths"));
        
    // ---------- Plots Initialization ----------
    TFileDirectory allDir=fs->mkdir("All/");
    TFileDirectory genDir=fs->mkdir("Gen/");
    TFileDirectory jetDir=fs->mkdir("Jets/");
    
    // Make TH1F -- unused
    std::vector<std::string> nLabels={"All (jets in Acc)", "Trigger", "# jets >4", "# med b-tag >0", "# med b-tag >1", "# med b-tag >2", "# med b-tag >4", "", "", ""};
    
    int nbins;
    float min, max;
    std::string name, title, opt;
    
    ifstream histFile(HistFile);
    if(!histFile.is_open()) {
        throw cms::Exception("HHAnalyzer Analyzer", HistFile + " file not found");
    }
    while(histFile >> name >> title >> nbins >> min >> max >> opt) {
        if(name.find('#')==std::string::npos) {
            while(title.find("~")!=std::string::npos) title=title.replace(title.find("~"), 1, " "); // Remove ~
            if(name.substr(0, 2)=="a_") Hist[name] = allDir.make<TH1F>(name.c_str(), title.c_str(), nbins, min, max); //.substr(2)
            if(name.substr(0, 2)=="g_") Hist[name] = genDir.make<TH1F>(name.c_str(), title.c_str(), nbins, min, max);
            if(name.substr(0, 2)=="j_") Hist[name] = jetDir.make<TH1F>(name.c_str(), title.c_str(), nbins, min, max);
            Hist[name]->Sumw2();
            Hist[name]->SetOption(opt.c_str());
            // Particular histograms
            if(name=="a_nEvents" || name=="e_nEvents" || name=="m_nEvents") for(unsigned int i=0; i<nLabels.size(); i++) Hist[name]->GetXaxis()->SetBinLabel(i+1, nLabels[i].c_str());
        }
    }
    histFile.close();

    std::cout << "---------- STARTING ----------" << std::endl;
}


HHAnalyzer::~HHAnalyzer() {
    // do anything here that needs to be done at desctruction time
    // (e.g. close files, deallocate resources etc.)
    std::cout << "---------- ENDING  ----------" << std::endl;
    
    delete theGenAnalyzer;
    delete thePileupAnalyzer;
    delete theTriggerAnalyzer;
    delete theElectronAnalyzer;
    delete theMuonAnalyzer;
    delete theJetAnalyzer;
}


// 
// member functions
//
// ------------ method called for each event  ------------
void HHAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {

    auto & filterPairs = EventInfo.filterPairs_;
    auto & weightPairs = EventInfo.weightPairs_;
    filterPairs.clear();
    weightPairs.clear();
    EventInfo.setEvent(iEvent.id().event());
    EventInfo.setLumiBlock(iEvent.luminosityBlock());
    EventInfo.setRun(iEvent.id().run());
    EventInfo.setIsMC(!iEvent.isRealData());

   
    EventWeight = TriggerWeight = LeptonWeight = 1.;
    PUWeight = PUWeightUp = PUWeightDown = 1.;
    PdfWeight = 1.;
    nPV = nElectrons = nMuons = nJets = nFatJets = nBTagJets = -1;

    Hist["a_nEvents"]->Fill(1., EventWeight);
    
    // -----------------------------------
    //           READ OBJECTS
    // -----------------------------------
    
    // Pu weight
    PUWeight     = thePileupAnalyzer->GetPUWeight(iEvent);
    PUWeightUp   = thePileupAnalyzer->GetPUWeightUp(iEvent);
    PUWeightDown = thePileupAnalyzer->GetPUWeightDown(iEvent);
    nPV = thePileupAnalyzer->GetPV(iEvent);
    Hist["a_nPVNoWeight"]->Fill(nPV, EventWeight);
    EventWeight *= PUWeight;
    Hist["a_nPVReWeight"]->Fill(nPV, EventWeight);
    
    // Trigger
    std::map<std::string, bool> TriggerMap;
    theTriggerAnalyzer->FillTriggerMap(iEvent, TriggerMap);
    EventWeight *= TriggerWeight;
    for (const auto & kv : TriggerMap) {
      filterPairs.emplace_back(kv.first, kv.second);
    } 
    
    // Electrons
    std::vector<pat::Electron> ElecVect = theElectronAnalyzer->FillElectronVector(iEvent);
    nElectrons = ElecVect.size();
    for(unsigned int i =0; i<ElecVect.size(); i++){
        if(ElecVect.at(i).userInt("isVeto")==1) nVetoElectrons++;
    }

    // Muons
    std::vector<pat::Muon> MuonVect = theMuonAnalyzer->FillMuonVector(iEvent);
    std::vector<pat::Muon> LooseMuonVect;
    for(unsigned int i =0; i<MuonVect.size(); i++){
      if(MuonVect.at(i).userInt("isLoose")==1){
	      LooseMuonVect.push_back(MuonVect.at(i));
	    }
    }
    // Jets
    std::vector<pat::Jet> JetsVect = theJetAnalyzer->FillJetVector(iEvent);
    theJetAnalyzer->CleanJetsFromMuons(JetsVect, MuonVect, 0.4);
    theJetAnalyzer->CleanJetsFromElectrons(JetsVect, ElecVect, 0.4);
    nJets = JetsVect.size();
    nBTagJets = theJetAnalyzer->GetNBJets(JetsVect);

    // Fat Jets
    //std::vector<pat::Jet> FatJetsVect = theFatJetAnalyzer->FillJetVector(iEvent);
    //nFatJets = FatJetsVect.size();

    // Missing Energy
    pat::MET MET = theJetAnalyzer->FillMetVector(iEvent);
    pat::MET Neutrino(MET);
    
    // -----------------------------------
    //           GEN LEVEL
    // -----------------------------------
    
    // Gen weights
    std::map<int, float> GenWeight = theGenAnalyzer->LHEWeightsMap(iEvent);
    EventWeight *= GenWeight[-1]; // apply default weight
     
    // LHE Particles
    std::map<std::string, float> LheMap = theGenAnalyzer->FillLheMap(iEvent);
    // Gen Particles
    std::vector<reco::GenParticle> GenPVect = theGenAnalyzer->FillGenVector(iEvent);    
    std::vector<reco::GenParticle> GenHsPart;
    std::vector<reco::GenParticle> GenBFromHsPart = theGenAnalyzer->PartonsFromDecays({25}, GenHsPart);
    std::vector<reco::GenParticle> TL_GenHsPart = theGenAnalyzer->FirstNGenParticlesWithDiffMother({25}, 2);
    std::vector<reco::GenParticle> TL_GenBFromHsPart = theGenAnalyzer->FirstNGenParticlesWithDiffMother({5,-5}, 4, 25);
    
    
    Hist["a_nEvents"]->Fill(2., EventWeight);
    
    // Jet variables
    theJetAnalyzer->AddVariables(JetsVect, MET);
    // Leptons
    theElectronAnalyzer->AddVariables(ElecVect, MET);
    theMuonAnalyzer->AddVariables(MuonVect, MET);

    // ---------- Fill objects ----------
    // reconstructed
    alp::convert(ElecVect, Electrons);
    alp::convert(MuonVect, Muons);
    alp::convert(JetsVect, Jets);
    alp::convert(MET, alp_MET);
    // gen level
    alp::convert(GenBFromHsPart, GenBFromHs);
    alp::convert(GenHsPart, GenHs);
    alp::convert(TL_GenHsPart, TL_GenHs);
    alp::convert(TL_GenBFromHsPart, TL_GenBFromHs);

    // fill weights
    weightPairs.emplace_back("EventWeight", EventWeight);
    weightPairs.emplace_back("PUWeight", PUWeight);
    weightPairs.emplace_back("PUWeightUp", PUWeightUp);
    weightPairs.emplace_back("PUWeightDown", PUWeightDown);
    weightPairs.emplace_back("TriggerWeight", TriggerWeight);
    weightPairs.emplace_back("LeptonWeight", LeptonWeight);
    weightPairs.emplace_back("LeptonWeightUp", LeptonWeightUp);
    weightPairs.emplace_back("LeptonWeightDown", LeptonWeightDown);

    // add all LHE weights ("lhe_weight_{id}",lhe_weight_{id})
    for (const auto & weight : GenWeight) {
      weightPairs.emplace_back("lhe_weight_"+std::to_string(weight.first), weight.second);
    }


    // fill sorting vectors
    j_sort_pt = std::vector<std::size_t>(Jets.size());
    std::iota(j_sort_pt.begin(), j_sort_pt.end(), 0);
    auto pt_comp = [&](std::size_t a, std::size_t b) {return Jets.at(a).pt() > Jets.at(b).pt();};
    std::sort(j_sort_pt.begin(), j_sort_pt.end(), pt_comp ); 

    j_sort_csv = std::vector<std::size_t>(Jets.size());
    std::iota(j_sort_csv.begin(), j_sort_csv.end(), 0);
    auto csv_comp = [&](std::size_t a, std::size_t b) {return Jets.at(a).CSV() > Jets.at(b).CSV();};
    std::sort(j_sort_csv.begin(), j_sort_csv.end(), csv_comp);

    j_sort_cmva = std::vector<std::size_t>(Jets.size());
    std::iota(j_sort_cmva.begin(), j_sort_cmva.end(), 0);
    auto cmva_comp = [&](std::size_t a, std::size_t b) {return Jets.at(a).CMVA() > Jets.at(b).CMVA();};
    std::sort(j_sort_cmva.begin(), j_sort_cmva.end(), cmva_comp);


    // --- Fill nEvents histogram --- no effective selection applied
    // --- num jets selection ---
    // commented to get faster execution
    //if(JetsVect.size() > 3) {
    //  Hist["a_nEvents"]->Fill(3., EventWeight);
    // --- b-Tag selection ---
    //  if( Jets.at(j_sort_csv[0]).CSV() > 0.800) Hist["a_nEvents"]->Fill(4., EventWeight);
    //  if( Jets.at(j_sort_csv[1]).CSV() > 0.800) Hist["a_nEvents"]->Fill(5., EventWeight);
    //  if( Jets.at(j_sort_csv[2]).CSV() > 0.800) Hist["a_nEvents"]->Fill(6., EventWeight);
    //  if( Jets.at(j_sort_csv[3]).CSV() > 0.800) Hist["a_nEvents"]->Fill(7., EventWeight);      
    //}
    
    // Fill tree
    tree->Fill();

}


// ------------ method called once each job just before starting event loop  ------------
void HHAnalyzer::beginJob() {
    
    // Create Tree and set Branches
    tree=fs->make<TTree>("tree", "tree");

    // branch to save event information 
    tree->Branch("EventInfo", &(EventInfo), 64000, 2);
    // save vector of electron, muon and jets
    tree->Branch("Electrons", &(Electrons), 64000, 2);
    tree->Branch("Muons", &(Muons), 64000, 2);
    tree->Branch("Jets", &(Jets), 64000, 2);
    tree->Branch("MET", &(alp_MET),64000,2);
    tree->Branch("GenBFromHs", &(GenBFromHs), 64000, 2);
    tree->Branch("GenHs", &(GenHs), 64000, 2);
    tree->Branch("TL_GenBFromHs", &(TL_GenBFromHs), 64000, 2);
    tree->Branch("TL_GenHs", &(TL_GenHs), 64000, 2);

    tree->Branch("j_sort_pt", &(j_sort_pt), 64000, 2);
    tree->Branch("j_sort_csv", &(j_sort_csv), 64000, 2);
    tree->Branch("j_sort_cmva", &(j_sort_cmva), 64000, 2);
}

// ------------ method called once each job just after ending the event loop  ------------
void HHAnalyzer::endJob() {
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void HHAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    //The following says we do not know what parameters are allowed so do no validation
    // Please change this to state exactly what you do use, even if it is no parameters
    edm::ParameterSetDescription desc;
    desc.setUnknown();
    descriptions.addDefault(desc);
}


//define this as a plug-in
DEFINE_FWK_MODULE(HHAnalyzer);
