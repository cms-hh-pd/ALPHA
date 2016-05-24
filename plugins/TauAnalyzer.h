#ifndef TAUANALYZER_H
#define TAUANALYZER_H

#include <iostream>
#include <fstream>
#include <cmath>
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/EDConsumerBase.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "DataFormats/PatCandidates/interface/Tau.h"
#include "RecoEgamma/EgammaTools/interface/ConversionTools.h"
#include "DataFormats/PatCandidates/interface/Conversion.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/VertexReco/interface/Vertex.h"

#include "TFile.h"
#include "TH2.h"

class TauAnalyzer {
    public:
        TauAnalyzer(edm::ParameterSet&, edm::ConsumesCollector&&);
        ~TauAnalyzer();
        virtual std::vector<pat::Tau> FillTauVector(const edm::Event&);
      
    private:
      
        edm::EDGetTokenT<std::vector<pat::Tau> > TauToken;
        edm::EDGetTokenT<reco::VertexCollection> VertexToken;
        int TauId;
        float TauPt, TauEta;

};


#endif
