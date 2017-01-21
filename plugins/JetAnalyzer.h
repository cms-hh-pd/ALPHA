#ifndef JETANALYZER_H
#define JETANALYZER_H

#include <iostream>
#include <fstream>
#include <cmath>
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/EDConsumerBase.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/MET.h"
#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/PatCandidates/interface/Electron.h"
#include "DataFormats/JetReco/interface/GenJetCollection.h"
#include "DataFormats/JetReco/interface/PFJetCollection.h"
#include "DataFormats/Candidate/interface/CandidateFwd.h"
#include "DataFormats/Common/interface/Association.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "RecoParticleFlow/PFProducer/interface/Utils.h"
#include "DataFormats/BTauReco/interface/SoftLeptonTagInfo.h"
#include "DataFormats/BTauReco/interface/SecondaryVertexTagInfo.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectorParameters.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectionUncertainty.h"
#include "DataFormats/Common/interface/ValueMap.h"
#include "JetMETCorrections/JetCorrector/interface/JetCorrector.h"
#include "CondFormats/JetMETObjects/interface/FactorizedJetCorrector.h"
#include "RecoilCorrector.h" // From: https://github.com/cms-met/MetTools/tree/master/RecoilCorrections
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/VertexReco/interface/Vertex.h"


#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/Candidate/interface/Candidate.h"
#include "JetMETCorrections/Modules/interface/JetResolution.h"
#include <CondFormats/JetMETObjects/interface/JetResolutionObject.h>

#include "BTagCalibrationStandalone.h"

#include "DataFormats/Common/interface/Ptr.h"

#include "TFile.h"
#include "TH2.h"
#include "TF1.h"
#include "TLorentzVector.h"
#include "TRandom3.h"

class JetAnalyzer {
    public:
        JetAnalyzer(edm::ParameterSet&, edm::ConsumesCollector&&);
        ~JetAnalyzer();
        virtual std::vector<pat::Jet> FillJetVector(const edm::Event&);
        virtual void CorrectJet(pat::Jet&, float, float, bool);
        virtual void CorrectMass(pat::Jet&, float, float, bool);
        virtual void CorrectPuppiMass(pat::Jet&, bool);
        virtual void CleanJetsFromMuons(std::vector<pat::Jet>&, std::vector<pat::Muon>&, float);
        virtual void CleanJetsFromElectrons(std::vector<pat::Jet>&, std::vector<pat::Electron>&, float);
        virtual void AddVariables(std::vector<pat::Jet>&, pat::MET&);
        virtual int GetNBJets(std::vector<pat::Jet>&);
        virtual pat::MET FillMetVector(const edm::Event&);
        virtual void ApplyRecoilCorrections(pat::MET&, const reco::Candidate::LorentzVector*, const reco::Candidate::LorentzVector*, int);
        //virtual float GetScaleUncertainty(pat::Jet&);
//         virtual float GetResolutionRatio(float);
//         virtual float GetResolutionErrorUp(float);
//         virtual float GetResolutionErrorDown(float);
        virtual bool isLooseJet(pat::Jet&);
        //virtual bool isMediumJet(pat::Jet&);
        virtual bool isTightJet(pat::Jet&);
        virtual bool isTightLepVetoJet(pat::Jet&);
        virtual std::vector<float> ReshapeBtagDiscriminator(pat::Jet&);
      
    private:
    
        edm::EDGetTokenT<std::vector<pat::Jet> > JetToken;
        edm::EDGetTokenT<std::vector<pat::MET> > MetToken;
        edm::EDGetTokenT<edm::ValueMap<float>> QGToken;
        int JetId;
        float Jet1Pt, Jet2Pt, JetEta;
        bool AddQG, RecalibrateJets, RecalibrateMass, RecalibratePuppiMass, SmearJets;
        int JECshift;
        std::string JECUncertaintyMC;
        std::string JECUncertaintyDATA;
        std::vector<std::string> JetCorrectorMC;
        std::vector<std::string> JetCorrectorDATA;
        std::vector<std::string> MassCorrectorMC;
        std::vector<std::string> MassCorrectorDATA;
        std::string MassCorrectorPuppi;
        edm::EDGetTokenT<reco::VertexCollection> VertexToken;        
        edm::EDGetTokenT<double> RhoToken;
        bool UseReshape;
        std::string BTag;
        int Jet1BTag, Jet2BTag;
        std::string BTagDB;
        bool UseRecoil;
        std::string RecoilMCFile;
        std::string RecoilDataFile;
        int JERshift;
        std::string JerName_res;
        std::string JerName_sf;
        float Rparameter;
        
        TFile* PuppiCorrFile;
        TF1* PuppiJECcorr_gen;
        TF1* PuppiJECcorr_reco_0eta1v3;
        TF1* PuppiJECcorr_reco_1v3eta2v5;
        
        // JEC Uncertainty
        JetCorrectionUncertainty* jecUncMC;
        JetCorrectionUncertainty* jecUncDATA;
        
        boost::shared_ptr<FactorizedJetCorrector> jetCorrMC;
        boost::shared_ptr<FactorizedJetCorrector> jetCorrDATA;
        boost::shared_ptr<FactorizedJetCorrector> massCorrMC;
        boost::shared_ptr<FactorizedJetCorrector> massCorrDATA;
        
        // Btag calibrations
        BTagCalibration       * calib;
    
        BTagCalibrationReader * reader;
        BTagCalibrationReader * reader_up_jes;
        BTagCalibrationReader * reader_down_jes;
//        BTagCalibrationReader * reader_up_lf;
//        BTagCalibrationReader * reader_up_hfstats1;
//        BTagCalibrationReader * reader_up_hfstats2;
//        BTagCalibrationReader * reader_up_cferr1;
//        BTagCalibrationReader * reader_up_cferr2;
//        BTagCalibrationReader * reader_down_lf;
//        BTagCalibrationReader * reader_down_hfstats1;
//        BTagCalibrationReader * reader_down_hfstats2;
//        BTagCalibrationReader * reader_down_cferr1;
//        BTagCalibrationReader * reader_down_cferr2;
        
        //JME
        JME::JetResolution              * resolution;
        JME::JetResolutionScaleFactor   * resolution_sf;        
        
        // Recoil corrections
        RecoilCorrector* recoilCorr;
};

#endif
