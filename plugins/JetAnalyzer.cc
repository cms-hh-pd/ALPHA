//#include "RecoilCorrector.hh" // From: https://github.com/cms-met/MetTools/tree/master/RecoilCorrections

#include "JetAnalyzer.h"


JetAnalyzer::JetAnalyzer(edm::ParameterSet& PSet, edm::ConsumesCollector&& CColl):
    JetToken(CColl.consumes<std::vector<pat::Jet> >(PSet.getParameter<edm::InputTag>("jets"))),
    MetToken(CColl.consumes<std::vector<pat::MET> >(PSet.getParameter<edm::InputTag>("met"))),
    QGToken(CColl.consumes<edm::ValueMap<float>>(edm::InputTag("QGTagger", "qgLikelihood"))),
    JetId(PSet.getParameter<int>("jetid")),
    Jet1Pt(PSet.getParameter<double>("jet1pt")),
    Jet2Pt(PSet.getParameter<double>("jet2pt")),
    JetEta(PSet.getParameter<double>("jeteta")),
    AddQG(PSet.getParameter<bool>("addQGdiscriminator")),
    RecalibrateJets(PSet.getParameter<bool>("recalibrateJets")),
    RecalibrateMass(PSet.getParameter<bool>("recalibrateMass")),
    RecalibratePuppiMass(PSet.getParameter<bool>("recalibratePuppiMass")),
    SmearJets(PSet.getParameter<bool>("smearJets")),
    JECshift(PSet.getParameter<int>("jecShift")),
    JECUncertaintyMC(PSet.getParameter<std::string>("jecUncertaintyMC")),
    JECUncertaintyDATA(PSet.getParameter<std::string>("jecUncertaintyDATA")),
    JetCorrectorMC(PSet.getParameter<std::vector<std::string> >("jecCorrectorMC")),
    JetCorrectorDATA(PSet.getParameter<std::vector<std::string> >("jecCorrectorDATA")),
    MassCorrectorMC(PSet.getParameter<std::vector<std::string> >("massCorrectorMC")),
    MassCorrectorDATA(PSet.getParameter<std::vector<std::string> >("massCorrectorDATA")),
    MassCorrectorPuppi(PSet.getParameter<std::string>("massCorrectorPuppi")),
    VertexToken(CColl.consumes<reco::VertexCollection>(PSet.getParameter<edm::InputTag>("vertices"))),
    RhoToken(CColl.consumes<double>(PSet.getParameter<edm::InputTag>("rho"))),
    UseReshape(PSet.getParameter<bool>("reshapeBTag")),
    BTag(PSet.getParameter<std::string>("btag")),
    Jet1BTag(PSet.getParameter<int>("jet1btag")),
    Jet2BTag(PSet.getParameter<int>("jet2btag")),
    BTagDB(PSet.getParameter<std::string>("btagDB")),
    UseRecoil(PSet.getParameter<bool>("metRecoil")),
    RecoilMCFile(PSet.getParameter<std::string>("metRecoilMC")),
    RecoilDataFile(PSet.getParameter<std::string>("metRecoilData")),
    JERshift(PSet.getParameter<int>("jerShift")),
    JerName_res(PSet.getParameter<std::string>("jerNameRes")),
    JerName_sf(PSet.getParameter<std::string>("jerNameSf"))
{
  
    jecUncMC = new JetCorrectionUncertainty(JECUncertaintyMC);
    jecUncDATA = new JetCorrectionUncertainty(JECUncertaintyDATA);
    
    
    if(RecalibrateJets) {
        std::vector<JetCorrectorParameters> jetParMC;
        for ( std::vector<std::string>::const_iterator payloadBegin = JetCorrectorMC.begin(), payloadEnd = JetCorrectorMC.end(), ipayload = payloadBegin; ipayload != payloadEnd; ++ipayload ) {
            //std::cout << *ipayload << "\n";
            jetParMC.push_back(JetCorrectorParameters(*ipayload));
        }    
        std::vector<JetCorrectorParameters> jetParDATA;
        for ( std::vector<std::string>::const_iterator payloadBegin = JetCorrectorDATA.begin(), payloadEnd = JetCorrectorDATA.end(), ipayload = payloadBegin; ipayload != payloadEnd; ++ipayload ) {
            //std::cout << *ipayload << "\n";
            jetParDATA.push_back(JetCorrectorParameters(*ipayload));
        }
        // Make the FactorizedJetCorrector
        jetCorrMC = boost::shared_ptr<FactorizedJetCorrector> ( new FactorizedJetCorrector(jetParMC) );
        jetCorrDATA = boost::shared_ptr<FactorizedJetCorrector> ( new FactorizedJetCorrector(jetParDATA) );
    }
    
    if(RecalibrateMass) {
        std::vector<JetCorrectorParameters> massParMC;
        for ( std::vector<std::string>::const_iterator payloadBegin = MassCorrectorMC.begin(), payloadEnd = MassCorrectorMC.end(), ipayload = payloadBegin; ipayload != payloadEnd; ++ipayload ) {
            massParMC.push_back(JetCorrectorParameters(*ipayload));
        }    
        std::vector<JetCorrectorParameters> massParDATA;
        for ( std::vector<std::string>::const_iterator payloadBegin = MassCorrectorDATA.begin(), payloadEnd = MassCorrectorDATA.end(), ipayload = payloadBegin; ipayload != payloadEnd; ++ipayload ) {
            massParDATA.push_back(JetCorrectorParameters(*ipayload));
        }
        // Make the FactorizedJetCorrector
        massCorrMC = boost::shared_ptr<FactorizedJetCorrector> ( new FactorizedJetCorrector(massParMC) );
        massCorrDATA = boost::shared_ptr<FactorizedJetCorrector> ( new FactorizedJetCorrector(massParDATA) );
    }
    
    if(SmearJets) {
        resolution    = new JME::JetResolution(JerName_res);
        resolution_sf = new JME::JetResolutionScaleFactor(JerName_sf);
        if (JerName_res.find("AK8") != std::string::npos)
            Rparameter = 0.8;
        else 
            Rparameter = 0.4;
    }
    
    if(RecalibratePuppiMass) {
        PuppiCorrFile = new TFile(MassCorrectorPuppi.c_str(), "READ");
        PuppiJECcorr_gen = (TF1*)PuppiCorrFile->Get("puppiJECcorr_gen");
        PuppiJECcorr_reco_0eta1v3 = (TF1*)PuppiCorrFile->Get("puppiJECcorr_reco_0eta1v3");
        PuppiJECcorr_reco_1v3eta2v5 = (TF1*)PuppiCorrFile->Get("puppiJECcorr_reco_1v3eta2v5");
    }
    
    // BTag calibrator
    if(UseReshape) {
        calib           = new BTagCalibration("CSVv2", BTagDB);
        reader          = new BTagCalibrationReader(calib,BTagEntry::OP_RESHAPING,"iterativefit","central");
        reader_up_jes   = new BTagCalibrationReader(calib,BTagEntry::OP_RESHAPING,"iterativefit","up_jes");
        reader_down_jes = new BTagCalibrationReader(calib,BTagEntry::OP_RESHAPING,"iterativefit","down_jes");
    //    reader_up_lf        = new BTagCalibrationReader(calib,BTagEntry::OP_RESHAPING,"iterativefit","up_lf");
    //    reader_up_hfstats1  = new BTagCalibrationReader(calib,BTagEntry::OP_RESHAPING,"iterativefit","up_hfstats1");
    //    reader_up_hfstats2  = new BTagCalibrationReader(calib,BTagEntry::OP_RESHAPING,"iterativefit","up_hfstats2");
    //    reader_up_cferr1    = new BTagCalibrationReader(calib,BTagEntry::OP_RESHAPING,"iterativefit","up_cferr1");
    //    reader_up_cferr2    = new BTagCalibrationReader(calib,BTagEntry::OP_RESHAPING,"iterativefit","up_cferr2");
    //    reader_down_lf        = new BTagCalibrationReader(calib,BTagEntry::OP_RESHAPING,"iterativefit","down_lf");
    //    reader_down_hfstats1  = new BTagCalibrationReader(calib,BTagEntry::OP_RESHAPING,"iterativefit","down_hfstats1");
    //    reader_down_hfstats2  = new BTagCalibrationReader(calib,BTagEntry::OP_RESHAPING,"iterativefit","down_hfstats2");
    //    reader_down_cferr1    = new BTagCalibrationReader(calib,BTagEntry::OP_RESHAPING,"iterativefit","down_cferr1");
    //    reader_down_cferr2    = new BTagCalibrationReader(calib,BTagEntry::OP_RESHAPING,"iterativefit","down_cferr2");
    }
    // Recoil Corrector
    if(UseRecoil) {
        recoilCorr = new RecoilCorrector(RecoilMCFile);
        recoilCorr->addDataFile(RecoilDataFile);
        recoilCorr->addMCFile(RecoilMCFile);
    }
    
    
    std::cout << " --- JetAnalyzer initialization ---" << std::endl;
    std::cout << "  jet Id            :\t" << JetId << std::endl;
    std::cout << "  jet pT [1, 2]     :\t" << Jet1Pt << "\t" << Jet2Pt << std::endl;
    std::cout << "  jet eta           :\t" << JetEta << std::endl;
    std::cout << "  b-tagging algo    :\t" << BTag << std::endl;
    std::cout << "  b-tag cut [1, 2]  :\t" << Jet1BTag << "\t" << Jet2BTag << std::endl;
    std::cout << "  apply recoil corr :\t" << (UseRecoil ? "YES" : "NO") << std::endl;
    std::cout << "  recoil file MC    :\t" << RecoilMCFile << std::endl;
    std::cout << "  recoil file Data  :\t" << RecoilDataFile << std::endl;
    std::cout << std::endl;
}

JetAnalyzer::~JetAnalyzer() {
    if(RecalibratePuppiMass) PuppiCorrFile->Close();

//    Creates segmentation fault (?)
//    if(UseReshape) {
//        delete reader;
//        delete reader_up_jes;
//        delete reader_down_jes;
//        delete calib;
//    }
    delete jecUncMC;
    delete jecUncDATA;
    if(UseRecoil) delete recoilCorr;
}





std::vector<pat::Jet> JetAnalyzer::FillJetVector(const edm::Event& iEvent) {
    bool isMC(!iEvent.isRealData());
    int BTagTh(Jet1BTag);
    float PtTh(Jet1Pt), EtaTh(JetEta);
    std::vector<pat::Jet> Vect;
    // Declare and open collection
    edm::Handle<std::vector<pat::Jet> > PFJetsCollection;
    iEvent.getByToken(JetToken, PFJetsCollection);
    
    // Open QG value maps
    edm::Handle<edm::ValueMap<float>> QGHandle;
    if(AddQG) iEvent.getByToken(QGToken, QGHandle);

    // Vertex collection
    edm::Handle<reco::VertexCollection> PVCollection;
    iEvent.getByToken(VertexToken, PVCollection);
    
    // Rho handle
    edm::Handle<double> rho_handle;
    iEvent.getByToken(RhoToken, rho_handle);
 
    // Loop on Jet collection
    for(std::vector<pat::Jet>::const_iterator it=PFJetsCollection->begin(); it!=PFJetsCollection->end(); ++it) {

        if(Vect.size()>0) {
            PtTh=Jet2Pt;
            BTagTh=Jet2BTag;
        }
        pat::Jet jet=*it;
        int idx=it-PFJetsCollection->begin();
        jet.addUserInt("Index", idx);
        pat::JetRef jetRef(PFJetsCollection, idx);

        if(RecalibrateJets) CorrectJet(jet, *rho_handle, PVCollection->size(), isMC);

        // JEC Uncertainty
        if (!isMC){
            jecUncDATA->setJetEta(jet.eta());
            jecUncDATA->setJetPt(jet.pt()); // here you must use the CORRECTED jet pt
            jet.addUserFloat("JESUncertainty", jecUncDATA->getUncertainty(true));
        } else {
            jecUncMC->setJetEta(jet.eta());
            jecUncMC->setJetPt(jet.pt()); // here you must use the CORRECTED jet pt
            jet.addUserFloat("JESUncertainty", jecUncMC->getUncertainty(true));
        }
        
        // PUPPI soft drop mass for AK8 jets
        if(jet.hasSubjets("SoftDropPuppi")) {
//            TLorentzVector puppiSoftdrop, puppiSoftdropSubjet;
//            auto const & sdSubjetsPuppi = jet.subjets("SoftDropPuppi");
//            for (auto const & it : sdSubjetsPuppi) {
//                puppiSoftdropSubjet.SetPtEtaPhiM(it->pt(), it->eta(), it->phi(), it->mass());
//                puppiSoftdrop += puppiSoftdropSubjet;
//            }
            reco::Particle::LorentzVector puppiSoftdrop;
            for (auto const & it : jet.subjets("SoftDropPuppi")) puppiSoftdrop += it->correctedP4(0);
            jet.addUserFloat("ak8PFJetsPuppiSoftDropPt", puppiSoftdrop.pt());
            jet.addUserFloat("ak8PFJetsPuppiSoftDropEta", puppiSoftdrop.eta());
            jet.addUserFloat("ak8PFJetsPuppiSoftDropPhi", puppiSoftdrop.phi());
            jet.addUserFloat("ak8PFJetsPuppiSoftDropEnergy", puppiSoftdrop.energy());
            jet.addUserFloat("ak8PFJetsPuppiSoftDropMass", puppiSoftdrop.mass());
            
            float tau21 = jet.userFloat("ak8PFJetsPuppiValueMap:NjettinessAK8PuppiTau2")/jet.userFloat("ak8PFJetsPuppiValueMap:NjettinessAK8PuppiTau1");
            float ddt = tau21 + 0.063 * log( jet.userFloat("ak8PFJetsPuppiSoftDropMass")*jet.userFloat("ak8PFJetsPuppiSoftDropMass")/jet.userFloat("ak8PFJetsPuppiSoftDropPt") );
            jet.addUserFloat("ddtTau21", ddt);
        }
        
        if(RecalibrateMass) CorrectMass(jet, *rho_handle, PVCollection->size(), isMC);
        if(RecalibratePuppiMass) CorrectPuppiMass(jet, isMC);
        
        // JER NEW IMPLEMENTATION
        if(SmearJets) {
            JME::JetParameters TheJetParameters;
            TheJetParameters.setJetPt(jet.pt());
            TheJetParameters.setJetEta(jet.pt());
            TheJetParameters.setRho(*rho_handle);

            float smearFactor = 1.;
            float smearFactorUp = 1.;
            float smearFactorDown = 1.;

            if(isMC) {
                float JERresolution = resolution->getResolution(TheJetParameters);
                float JERsf         = resolution_sf->getScaleFactor(TheJetParameters);
                float JERsfUp       = resolution_sf->getScaleFactor(TheJetParameters, Variation::UP);
                float JERsfDown     = resolution_sf->getScaleFactor(TheJetParameters, Variation::DOWN);
//                 std::cout << "JERresolution " << JERresolution << "\n";
//                 std::cout << "JERsf         " << JERsf << "\n";
//                 std::cout << "JERsfUp       " << JERsfUp << "\n";
//                 std::cout << "JERsfDown     " << JERsfDown << "\n";
                const reco::GenJet* genJet=jet.genJet();
                if(genJet) {
                    if ( ( sqrt( pow(jet.eta() - genJet->eta(),2) + pow(jet.phi() - genJet->phi(),2) ) < 0.5*Rparameter )  &&
                         fabs( jet.pt() - genJet->pt()) < 3.*JERresolution*genJet->pt() ) { // (DeltaR < R/2) AND (DeltaPt < 3*PtRes)
                        smearFactor = max(0.,genJet->pt()+JERsf*(jet.pt() - genJet->pt()))/jet.pt();
                        smearFactorUp = max(0.,genJet->pt()+JERsfUp*(jet.pt() - genJet->pt()))/jet.pt();
                        smearFactorDown = max(0.,genJet->pt()+JERsfDown*(jet.pt() - genJet->pt()))/jet.pt();
                    }  
                    else {
                        TRandom3 rnd(0);
                        smearFactor = rnd.Gaus(1.,sqrt(JERsf*JERsf-1.)*JERresolution*genJet->pt());
                        smearFactorUp = rnd.Gaus(1.,sqrt(JERsfUp*JERsfUp-1.)*JERresolution*genJet->pt());
                        smearFactorDown = rnd.Gaus(1.,sqrt(JERsfDown*JERsfDown-1.)*JERresolution*genJet->pt());
                    }
                }
            }        
//             std::cout << "Rparameter      " << Rparameter << "\n";
//             std::cout << "smearFactor     " << smearFactor << "\n";
//             std::cout << "smearFactorUp   " << smearFactorUp << "\n";
//             std::cout << "smearFactorDown " << smearFactorDown << "\n";

            if(JERshift == -1) jet.setP4(jet.p4() * smearFactorDown);
            else if (JERshift == 1) jet.setP4(jet.p4() * smearFactorUp);
            else jet.setP4(jet.p4() * smearFactor);

            jet.addUserFloat("JERUncertainty", smearFactor);
            jet.addUserFloat("JERUncertaintyUp", smearFactorUp);
            jet.addUserFloat("JERUncertaintyDown", smearFactorDown);           
        }        
        // JER NEW IMPLEMENTATION        

//         // (very)OLD Jet Energy Smearing
//         if(isMC) {
//             const reco::GenJet* genJet=jet.genJet();
//             if(genJet) {
//                 float smearFactor=GetResolutionRatio(jet.eta());
//                 reco::Candidate::LorentzVector smearedP4;
//                 smearedP4=jet.p4()-genJet->p4();
//                 smearedP4*=smearFactor; // +- 3*smearFactorErr;
//                 smearedP4+=genJet->p4();
//                 jet.setP4(smearedP4);
//             }
//         }
        
        // Pt and eta cut
        if(jet.pt()<PtTh || fabs(jet.eta())>EtaTh) continue;
        // Quality cut
        if(JetId==1 && !isLooseJet(jet)) continue;
        if(JetId==2 && !isTightJet(jet)) continue;
        if(JetId==3 && !isTightLepVetoJet(jet)) continue;
        // b-tagging
        if(BTagTh==1 && jet.bDiscriminator(BTag)<BTagTh) continue;
        // Fill jet scale uncertainty
        jet.addUserInt("isLoose", isLooseJet(jet) ? 1 : 0);
        jet.addUserInt("isTight", isTightJet(jet) ? 1 : 0);
        jet.addUserInt("isTightLepVeto", isTightLepVetoJet(jet) ? 1 : 0);
        jet.addUserFloat("ReshapedDiscriminator", ReshapeBtagDiscriminator(jet)[0]);
        jet.addUserFloat("ReshapedDiscriminatorUp", ReshapeBtagDiscriminator(jet)[1]);
        jet.addUserFloat("ReshapedDiscriminatorDown", ReshapeBtagDiscriminator(jet)[2]);

        // CSV reshaping for soft drop subjets
        if(jet.hasSubjets("SoftDrop")) {
            auto const & sdSubjets = jet.subjets("SoftDrop");
            short nsj = 1;
            for (auto const & it : sdSubjets) {
                pat::Jet subjet = it;
                jet.addUserFloat(Form("ReshapedDiscriminator%d",nsj), ReshapeBtagDiscriminator(subjet)[0]);
                jet.addUserFloat(Form("ReshapedDiscriminatorUp%d",nsj), ReshapeBtagDiscriminator(subjet)[1]);
                jet.addUserFloat(Form("ReshapedDiscriminatorDown%d",nsj), ReshapeBtagDiscriminator(subjet)[2]);
                ++nsj;
            }
        }
        
        //QG tagger for AK4 jets
        if(AddQG && jet.nSubjetCollections()<=0) {
            jet.addUserFloat("QGLikelihood", (*QGHandle)[jetRef]);
        }
        
        Vect.push_back(jet); // Fill vector
    }
    return Vect;
}


void JetAnalyzer::CorrectJet(pat::Jet& jet, float rho, float nPV, bool isMC) {
    double corr(1.);
    reco::Candidate::LorentzVector uncorrJet = jet.correctedP4(0);
    
    if(!isMC) {
        jetCorrDATA->setJetEta( uncorrJet.Eta() );
        jetCorrDATA->setJetPt ( uncorrJet.Pt() );
        jetCorrDATA->setJetE  ( uncorrJet.E() );
        jetCorrDATA->setJetA  ( jet.jetArea() );
        jetCorrDATA->setRho   ( rho );
        jetCorrDATA->setNPV   ( nPV );
        corr = jetCorrDATA->getCorrection();
    }
    else {
        jetCorrMC->setJetEta( uncorrJet.Eta() );
        jetCorrMC->setJetPt ( uncorrJet.Pt() );
        jetCorrMC->setJetE  ( uncorrJet.E() );
        jetCorrMC->setJetA  ( jet.jetArea() );
        jetCorrMC->setRho   ( rho );
        jetCorrMC->setNPV   ( nPV );
        corr = jetCorrMC->getCorrection();
    }
    
    reco::Candidate::LorentzVector corrJet(uncorrJet);
    jet.setP4(corrJet * corr);
}

void JetAnalyzer::CorrectMass(pat::Jet& jet, float rho, float nPV, bool isMC) {
    double corr(1.);
    reco::Candidate::LorentzVector uncorrJet = jet.correctedP4(0);
    
    if(!isMC) {
        jetCorrDATA->setJetEta( uncorrJet.Eta() );
        jetCorrDATA->setJetPt ( uncorrJet.Pt() );
        jetCorrDATA->setJetE  ( uncorrJet.E() );
        jetCorrDATA->setJetA  ( jet.jetArea() );
        jetCorrDATA->setRho   ( rho );
        jetCorrDATA->setNPV   ( nPV );
        corr = jetCorrDATA->getCorrection();
    }
    else {
        jetCorrMC->setJetEta( uncorrJet.Eta() );
        jetCorrMC->setJetPt ( uncorrJet.Pt() );
        jetCorrMC->setJetE  ( uncorrJet.E() );
        jetCorrMC->setJetA  ( jet.jetArea() );
        jetCorrMC->setRho   ( rho );
        jetCorrMC->setNPV   ( nPV );
        corr = jetCorrMC->getCorrection();
    }
    if(jet.hasUserFloat("ak8PFJetsCHSPrunedMass")) jet.addUserFloat("ak8PFJetsCHSPrunedMassCorr", jet.userFloat("ak8PFJetsCHSPrunedMass") * corr);
    if(jet.hasUserFloat("ak8PFJetsCHSSoftDropMass")) jet.addUserFloat("ak8PFJetsCHSSoftDropMassCorr", jet.userFloat("ak8PFJetsCHSSoftDropMass") * corr);
    //if(jet.hasUserFloat("ak8PFJetsPuppiSoftDropMass")) jet.addUserFloat("ak8PFJetsPuppiSoftDropMassCorr", jet.userFloat("ak8PFJetsPuppiSoftDropMass") * corr);
}


void JetAnalyzer::CorrectPuppiMass(pat::Jet& jet, bool isMC) {
    bool hasInfo( jet.hasUserFloat("ak8PFJetsPuppiSoftDropMass") && jet.hasUserFloat("ak8PFJetsPuppiSoftDropPt") && jet.hasUserFloat("ak8PFJetsPuppiSoftDropEta") );
    float corr(1.), genCorr(1.), recoCorr(1.);
    if(hasInfo && jet.userFloat("ak8PFJetsPuppiSoftDropMass") > 0.) {
        genCorr = PuppiJECcorr_gen->Eval( jet.userFloat("ak8PFJetsPuppiSoftDropPt") );
        if(fabs(jet.userFloat("ak8PFJetsPuppiSoftDropEta")) <= 1.3) 
            recoCorr = PuppiJECcorr_reco_0eta1v3->Eval( jet.userFloat("ak8PFJetsPuppiSoftDropPt") );
        else if(fabs(jet.userFloat("ak8PFJetsPuppiSoftDropEta")) > 1.3 ) 
            recoCorr = PuppiJECcorr_reco_1v3eta2v5->Eval( jet.userFloat("ak8PFJetsPuppiSoftDropPt") );
        corr = genCorr * recoCorr;
    }
    if(corr < 0.) corr = 0.;
    jet.addUserFloat("ak8PFJetsPuppiSoftDropMassCorr", jet.userFloat("ak8PFJetsPuppiSoftDropMass") * corr);

    if(isMC){
        float JMSSf  = 1.;
        float JMSUnc = 0.004;
        float JESUnc = jet.userFloat("JESUncertainty");    
        jet.addUserFloat("ak8PFJetsPuppiSoftDropMassCorrJMS", jet.userFloat("ak8PFJetsPuppiSoftDropMassCorr")       * JMSSf);
        jet.addUserFloat("ak8PFJetsPuppiSoftDropMassCorrJMSUp", jet.userFloat("ak8PFJetsPuppiSoftDropMassCorr")     * (JMSSf + sqrt(JMSUnc*JMSUnc + JESUnc*JESUnc) ) );
        jet.addUserFloat("ak8PFJetsPuppiSoftDropMassCorrJMSDown", jet.userFloat("ak8PFJetsPuppiSoftDropMassCorr")   * (JMSSf - sqrt(JMSUnc*JMSUnc + JESUnc*JESUnc) ) );
        
        float JMRSf   = 1.08;
        float JMRUnc  = 0.11;
        TRandom3 rnd(0);
        float smearJMR    = rnd.Gaus(1.,JMRSf-1.);        
        jet.addUserFloat("ak8PFJetsPuppiSoftDropMassCorrJMR", jet.userFloat("ak8PFJetsPuppiSoftDropMassCorr")       * smearJMR);
        jet.addUserFloat("ak8PFJetsPuppiSoftDropMassCorrJMRUp", jet.userFloat("ak8PFJetsPuppiSoftDropMassCorr")     * smearJMR * (1. + JMRUnc) );
        jet.addUserFloat("ak8PFJetsPuppiSoftDropMassCorrJMRDown", jet.userFloat("ak8PFJetsPuppiSoftDropMassCorr")   * smearJMR * (1. - JMRUnc) );
    }
}


void JetAnalyzer::CleanJetsFromMuons(std::vector<pat::Jet>& Jets, std::vector<pat::Muon>& Muons, float angle) {
    for(unsigned int m = 0; m < Muons.size(); m++) {
        for(unsigned int j = 0; j < Jets.size(); ) {
            if(deltaR(Jets[j], Muons[m]) < angle) Jets.erase(Jets.begin() + j);
            else j++;
        }
    }
}

void JetAnalyzer::CleanJetsFromElectrons(std::vector<pat::Jet>& Jets, std::vector<pat::Electron>& Electrons, float angle) {
    for(unsigned int e = 0; e < Electrons.size(); e++) {
        for(unsigned int j = 0; j < Jets.size(); ) {
            if(deltaR(Jets[j], Electrons[e]) < angle) Jets.erase(Jets.begin() + j);
            else j++;
        }
    }
}

void JetAnalyzer::AddVariables(std::vector<pat::Jet>& Jets, pat::MET& MET) {
    for(unsigned int j = 0; j < Jets.size(); j++) {
        Jets[j].addUserFloat("dPhi_met", fabs(reco::deltaPhi(Jets[j].phi(), MET.phi())));
        Jets[j].addUserFloat("dPhi_Jet1", fabs(reco::deltaPhi(Jets[j].phi(), Jets[0].phi())));
    }
}

int JetAnalyzer::GetNBJets(std::vector<pat::Jet>& Jets) {
    int n(0);
    for(unsigned int i = 0; i < Jets.size(); i++) if(abs(Jets[i].hadronFlavour()) == 5) n++;
    return n;
}





pat::MET JetAnalyzer::FillMetVector(const edm::Event& iEvent) {
    
    edm::Handle<std::vector<pat::MET> > MetCollection;
    iEvent.getByToken(MetToken, MetCollection);
    pat::MET MEt = MetCollection->front();
    MEt.addUserFloat("ptRaw", MEt.uncorPt());
    MEt.addUserFloat("phiRaw", MEt.uncorPhi());
    MEt.addUserFloat("ptType1", MEt.pt());
    MEt.addUserFloat("phiType1", MEt.phi());
    return MEt;
}

void JetAnalyzer::ApplyRecoilCorrections(pat::MET& MET, const reco::Candidate::LorentzVector* GenV, const reco::Candidate::LorentzVector* RecoV, int nJets) {
    double MetPt(MET.pt()), MetPhi(MET.phi()), MetPtScaleUp(MET.pt()), MetPhiScaleUp(MET.phi()), MetPtScaleDown(MET.pt()), MetPhiScaleDown(MET.phi()), MetPtResUp(MET.pt()), MetPhiResUp(MET.phi()), MetPtResDown(MET.pt()), MetPhiResDown(MET.phi());
    double GenPt(0.), GenPhi(0.), LepPt(0.), LepPhi(0.), LepPx(0.), LepPy(0.);
    double RecoilX(0.), RecoilY(0.), Upara(0.), Uperp(0.);
    
    if(GenV) {
        GenPt = GenV->pt();
        GenPhi = GenV->phi();
    }
    else {
        throw cms::Exception("JetAnalyzer", "GenV boson is null. No Recoil Correction can be derived");
        return;
    }
    
    if(RecoV) {
        LepPt = RecoV->pt();
        LepPhi = RecoV->phi();
        LepPx = RecoV->px();
        LepPy = RecoV->py();
        RecoilX = - MET.px() - LepPx;
        RecoilY = - MET.py() - LepPy;
        Upara = (RecoilX*LepPx + RecoilY*LepPy) / LepPt;
        Uperp = (RecoilX*LepPy - RecoilY*LepPx) / LepPt;
    }
    
    // Apply Recoil Corrections
    if(UseRecoil) {
        recoilCorr->CorrectType2(MetPt,          MetPhi,          GenPt, GenPhi, LepPt, LepPhi, Upara, Uperp,  0,  0, nJets);
        recoilCorr->CorrectType2(MetPtScaleUp,   MetPhiScaleUp,   GenPt, GenPhi, LepPt, LepPhi, Upara, Uperp,  3,  0, nJets);
        recoilCorr->CorrectType2(MetPtScaleDown, MetPhiScaleDown, GenPt, GenPhi, LepPt, LepPhi, Upara, Uperp, -3,  0, nJets);
        recoilCorr->CorrectType2(MetPtResUp,     MetPhiResUp,     GenPt, GenPhi, LepPt, LepPhi, Upara, Uperp,  0,  3, nJets);
        recoilCorr->CorrectType2(MetPtResDown,   MetPhiResDown,   GenPt, GenPhi, LepPt, LepPhi, Upara, Uperp,  0, -3, nJets);
    }
    
    // Set userFloats for systematics
    MET.addUserFloat("ptScaleUp", MetPtScaleUp);
    MET.addUserFloat("ptScaleDown", MetPtScaleDown);
    MET.addUserFloat("ptResUp", MetPtResUp);
    MET.addUserFloat("ptResDown", MetPtResDown);
    
    // Set new P4
    MET.setP4(reco::Candidate::PolarLorentzVector(MetPt, MET.eta(), MetPhi, MET.mass()));
}


// // https://twiki.cern.ch/twiki/bin/view/CMS/JetResolution
// float JetAnalyzer::GetResolutionRatio(float eta) {
//     eta=fabs(eta);
//     if(eta>=0.0 && eta<0.5) return 1.122; 
//     if(eta>=0.5 && eta<0.8) return 1.167;
//     if(eta>=0.8 && eta<1.1) return 1.168;
//     if(eta>=1.1 && eta<1.3) return 1.029;
//     if(eta>=1.3 && eta<1.7) return 1.115;
//     if(eta>=1.7 && eta<1.9) return 1.041;
//     if(eta>=1.9 && eta<2.1) return 1.167;
//     if(eta>=2.1 && eta<2.3) return 1.094;
//     if(eta>=2.3 && eta<2.5) return 1.168;
//     if(eta>=2.5 && eta<2.8) return 1.266;
//     if(eta>=2.8 && eta<3.0) return 1.595;
//     if(eta>=3.0 && eta<3.2) return 0.998;
//     if(eta>=3.2 && eta<5.0) return 1.226;
//     return -1.;
// }
// float JetAnalyzer::GetResolutionErrorUp(float eta) {
//     eta=fabs(eta);
//     if(eta>=0.0 && eta<0.5) return 1.122 + 0.026; 
//     if(eta>=0.5 && eta<0.8) return 1.167 + 0.048;
//     if(eta>=0.8 && eta<1.1) return 1.168 + 0.046;
//     if(eta>=1.1 && eta<1.3) return 1.029 + 0.066;
//     if(eta>=1.3 && eta<1.7) return 1.115 + 0.030;
//     if(eta>=1.7 && eta<1.9) return 1.041 + 0.062;
//     if(eta>=1.9 && eta<2.1) return 1.167 + 0.086;
//     if(eta>=2.1 && eta<2.3) return 1.094 + 0.093;
//     if(eta>=2.3 && eta<2.5) return 1.168 + 0.120;
//     if(eta>=2.5 && eta<2.8) return 1.266 + 0.132;
//     if(eta>=2.8 && eta<3.0) return 1.595 + 0.175;
//     if(eta>=3.0 && eta<3.2) return 0.998 + 0.066;
//     if(eta>=3.2 && eta<5.0) return 1.226 + 0.145;
//     return -1.;
// }
// float JetAnalyzer::GetResolutionErrorDown(float eta) {
//     eta=fabs(eta);
//     if(eta>=0.0 && eta<0.5) return 1.122 - 0.026; 
//     if(eta>=0.5 && eta<0.8) return 1.167 - 0.048;
//     if(eta>=0.8 && eta<1.1) return 1.168 - 0.046;
//     if(eta>=1.1 && eta<1.3) return 1.029 - 0.066;
//     if(eta>=1.3 && eta<1.7) return 1.115 - 0.030;
//     if(eta>=1.7 && eta<1.9) return 1.041 - 0.062;
//     if(eta>=1.9 && eta<2.1) return 1.167 - 0.086;
//     if(eta>=2.1 && eta<2.3) return 1.094 - 0.093;
//     if(eta>=2.3 && eta<2.5) return 1.168 - 0.120;
//     if(eta>=2.5 && eta<2.8) return 1.266 - 0.132;
//     if(eta>=2.8 && eta<3.0) return 1.595 - 0.175;
//     if(eta>=3.0 && eta<3.2) return 0.998 - 0.066;
//     if(eta>=3.2 && eta<5.0) return 1.226 - 0.145;
//     return -1.;
// }

// PFJet Quality ID 2015-2016: see https://twiki.cern.ch/twiki/bin/viewauth/CMS/JetID#Recommendations_for_13_TeV_data
bool JetAnalyzer::isLooseJet(pat::Jet& jet) {
    if(fabs(jet.eta())<=2.7){ /// |eta| < 2.7
        if(jet.neutralHadronEnergyFraction()>=0.99) return false;
        if(jet.neutralEmEnergyFraction()>=0.99) return false;
        if((jet.chargedMultiplicity()+jet.neutralMultiplicity())<=1) return false;
        if(fabs(jet.eta())<=2.4) { /// |eta| < 2.4
            if(jet.chargedHadronEnergyFraction()<=0.) return false;
            if(jet.chargedMultiplicity()<=0) return false;
            if(jet.chargedEmEnergyFraction()>=0.99) return false;
        }
    }
    else{ /// |eta| > 2.7
        if(jet.neutralEmEnergyFraction()>=0.90) return false;
        if (fabs(jet.eta())<=3.0) { /// 2.7 < |eta| < 3.0
            if(jet.neutralMultiplicity()<=2) return false;
        }
        else{ /// |eta| > 3.0
            if(jet.neutralMultiplicity()<=10) return false;
        }
    }
    return true;
}

bool JetAnalyzer::isTightJet(pat::Jet& jet) {
    if(fabs(jet.eta())<=2.7){ /// |eta| < 2.7
        if(jet.neutralHadronEnergyFraction()>=0.90) return false;
        if(jet.neutralEmEnergyFraction()>=0.90) return false;
        if((jet.chargedMultiplicity()+jet.neutralMultiplicity())<=1) return false;
        if(fabs(jet.eta())<=2.4) { /// |eta| < 2.4
            if(jet.chargedHadronEnergyFraction()<=0.) return false;
            if(jet.chargedMultiplicity()<=0) return false;
            if(jet.chargedEmEnergyFraction()>=0.99) return false;
        }
    }
    else{ /// |eta| > 2.7
        if(jet.neutralEmEnergyFraction()>=0.90) return false;
        if (fabs(jet.eta())<=3.0) { /// 2.7 < |eta| < 3.0
            if(jet.neutralMultiplicity()<=2) return false;
        }
        else{ /// |eta| > 3.0
            if(jet.neutralMultiplicity()<=10) return false;
        }
    }
    return true;
}

bool JetAnalyzer::isTightLepVetoJet(pat::Jet& jet) {
    if(fabs(jet.eta())<=2.7){ /// |eta| < 2.7
        if(jet.neutralHadronEnergyFraction()>=0.90) return false;
        if(jet.neutralEmEnergyFraction()>=0.90) return false;
        if((jet.chargedMultiplicity()+jet.neutralMultiplicity())<=1) return false;
        if(jet.muonEnergyFraction()>=0.80) return false;
        if(fabs(jet.eta())<=2.4) { /// |eta| < 2.4
            if(jet.chargedHadronEnergyFraction()<=0.) return false;
            if(jet.chargedMultiplicity()<=0) return false;
            if(jet.chargedEmEnergyFraction()>=0.99) return false;
        }
    }
    else{ /// |eta| > 2.7
        if(jet.neutralEmEnergyFraction()>=0.90) return false;
        if (fabs(jet.eta())<=3.0) { /// 2.7 < |eta| < 3.0
            if(jet.neutralMultiplicity()<=2) return false;
        }
        else{ /// |eta| > 3.0
            if(jet.neutralMultiplicity()<=10) return false;
        }
    }
    return true;
}

std::vector<float> JetAnalyzer::ReshapeBtagDiscriminator(pat::Jet& jet) {
    float pt(jet.pt()), eta(fabs(jet.eta())), discr(jet.bDiscriminator(BTag));
    int hadronFlavour_ = std::abs(jet.hadronFlavour());
    std::vector<float> reshapedDiscr = {discr,discr,discr};
    
    if(UseReshape) {
        BTagEntry::JetFlavor jf = BTagEntry::FLAV_UDSG;
        if (hadronFlavour_ == 5) jf = BTagEntry::FLAV_B;
        else if (hadronFlavour_ == 4) jf = BTagEntry::FLAV_C;
        else if (hadronFlavour_ == 0) jf = BTagEntry::FLAV_UDSG;

        reshapedDiscr[0] = discr*reader->eval(jf, eta, pt, discr); 
        reshapedDiscr[1] = discr*reader_up_jes->eval(jf, eta, pt, discr); 
        reshapedDiscr[2] = discr*reader_down_jes->eval(jf, eta, pt, discr); 
    
//     float reshapedDiscr_up_jes        = discr*reader_up_jes->eval(jf, eta, pt); 
//     float reshapedDiscr_up_lf         = discr*reader_up_lf->eval(jf, eta, pt); 
//     float reshapedDiscr_up_hfstats1   = discr*reader_up_hfstats1->eval(jf, eta, pt); 
//     float reshapedDiscr_up_hfstats2   = discr*reader_up_hfstats2->eval(jf, eta, pt); 
//     float reshapedDiscr_up_cferr1     = discr*reader_up_cferr1->eval(jf, eta, pt); 
//     float reshapedDiscr_up_cferr2     = discr*reader_up_cferr2->eval(jf, eta, pt); 
//     float reshapedDiscr_down_jes        = discr*reader_down_jes->eval(jf, eta, pt); 
//     float reshapedDiscr_down_lf         = discr*reader_down_lf->eval(jf, eta, pt); 
//     float reshapedDiscr_down_hfstats1   = discr*reader_down_hfstats1->eval(jf, eta, pt); 
//     float reshapedDiscr_down_hfstats2   = discr*reader_down_hfstats2->eval(jf, eta, pt); 
//     float reshapedDiscr_down_cferr1     = discr*reader_down_cferr1->eval(jf, eta, pt); 
//     float reshapedDiscr_down_cferr2     = discr*reader_down_cferr2->eval(jf, eta, pt); 
    
//     std::cout << Form("pt, eta, b-tag, flav, reshapedDiscr      : %f, %f, %f, %d, %f\n", pt, eta, discr, jf, reshapedDiscr);
//     std::cout << Form(" jes     : %f / %f\n", reshapedDiscr_up_jes, reshapedDiscr_down_jes);
//     std::cout << Form(" lf      : %f / %f\n", reshapedDiscr_up_lf, reshapedDiscr_down_lf);
//     std::cout << Form(" hfstats1: %f / %f\n", reshapedDiscr_up_hfstats1, reshapedDiscr_down_hfstats1);
//     std::cout << Form(" hfstats2: %f / %f\n", reshapedDiscr_up_hfstats2, reshapedDiscr_down_hfstats2);
//     std::cout << Form(" cferr1  : %f / %f\n", reshapedDiscr_up_cferr1, reshapedDiscr_down_cferr1);
//     std::cout << Form(" cferr2  : %f / %f\n", reshapedDiscr_up_cferr2, reshapedDiscr_down_cferr2);
    }
    return reshapedDiscr;
}


/*
bool JetAnalyzer::isMediumJet(pat::Jet& jet) {
    if(jet.neutralHadronEnergyFraction()>0.95) return false;
    if(jet.neutralEmEnergyFraction()>0.95) return false;
    if(jet.numberOfDaughters()<=1) return false;
    if(fabs(jet.eta())<2.4) {
      if(jet.chargedHadronEnergyFraction()<=0.) return false;
      if(jet.chargedEmEnergyFraction()>0.99) return false;
      if(jet.chargedMultiplicity()<=0) return false;
    }
    return true;
}
bool JetAnalyzer::isTightJet(pat::Jet& jet) {
    if(jet.neutralHadronEnergyFraction()>0.90) return false;
    if(jet.neutralEmEnergyFraction()>0.90) return false;
    if(jet.numberOfDaughters()<=1) return false;
    if(fabs(jet.eta())<2.4) {
      if(jet.chargedHadronEnergyFraction()<=0.) return false;
      if(jet.chargedEmEnergyFraction()>0.99) return false;
      if(jet.chargedMultiplicity()<=0) return false;
    }
    return true;
}

*/


