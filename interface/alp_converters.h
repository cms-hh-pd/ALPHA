
#pragma once

#include <vector>
#include "Math/Vector4D.h"
#include "Math/Vector4Dfwd.h"

#include "alp_objects.h"

namespace alp {

  void convert(const pat::Jet & orig, alp::Jet & dest) {

    dest.p4_ = orig.p4();

    // generator/MC inforamtion
    if (orig.genJet()) {
      dest.genJets_.emplace_back(orig.genJet()->p4());
    }
    if (orig.genParton()) {
      dest.genPartons_.emplace_back(orig.genParton()->p4());
    }
    dest.partonFlavour_ = orig.partonFlavour();
    dest.hadronFlavour_ = orig.hadronFlavour();
    dest.ptRaw_ = orig.correctedJet(0).pt();
    dest.JESunc_ = orig.userFloat("JESUncertainty");   
    dest.JERuncUp_ = orig.userFloat("JERUncertaintyUp");
    dest.JERuncDown_ = orig.userFloat("JERUncertaintyDown"); 
    dest.puId_ = orig.userFloat("pileupJetId:fullDiscriminant");
    dest.mult_ = orig.chargedMultiplicity() + orig.neutralMultiplicity();
    dest.chm_ = orig.chargedHadronMultiplicity();
    dest.chf_ = orig.chargedHadronEnergyFraction();
    dest.nhf_ = orig.neutralHadronEnergyFraction();
    dest.muf_ = orig.muonEnergyFraction();

    // intDiscriminators - ids
    std::vector<std::string> ids = { "isLoose",
                                     //"isMedium", #unknown
                                     "isTight",  
                                     "isTightLepVeto",
                                     "pileupJetId:fullId"
                                    };
    for (const auto & iD : ids) {
      dest.ids_.emplace_back(iD, orig.userInt(iD));
    }
    // bDiscriminators
    std::vector<std::string> bDiscs = { "pfCombinedInclusiveSecondaryVertexV2BJetTags",
                                        "pfCombinedMVAV2BJetTags" };
    for (const auto & bDisc : bDiscs) {
      dest.discs_.emplace_back(bDisc, orig.bDiscriminator(bDisc));
    }
    // other discriminators (userFloats) 
    std::vector<std::string> oDiscs = { "ReshapedDiscriminator",
                                        "ReshapedDiscriminatorUp",
                                        "ReshapedDiscriminatorDown",
                                        "CMVAR",
                                        "CMVARUp",
                                        "CMVARDown",
                                        "QGLikelihood"
                                       };
    for (const auto & oDisc : oDiscs) {
      if (orig.hasUserFloat(oDisc)) {
        dest.discs_.emplace_back(oDisc, orig.userFloat(oDisc));
      }
    }
  }


  void convert(const std::vector<pat::Jet> & orig, std::vector<alp::Jet> & dest) {
    dest.clear();
    for (const auto & e : orig) {
      dest.emplace_back();
      convert(e,dest.back());
    }
  }

  void convert(const pat::Electron & orig, alp::Lepton & dest) {

    // four-momenta
    dest.p4_ = orig.p4();
    // other variables (isolation only for the time being
    dest.iso03_ = orig.hasUserFloat("pfIso03") ? orig.userFloat("pfIso03") : -1.;

  }

  void convert(const std::vector<pat::Electron> & orig, std::vector<alp::Lepton> & dest) {
    dest.clear();
    for (const auto & e : orig) {
      dest.emplace_back();
      convert(e,dest.back());
    }
  }

  void convert(const pat::Muon & orig, alp::Lepton & dest) {

    // four-momenta
    dest.p4_ = orig.p4();
    // other variables (isolation only for the time being
    dest.iso03_ = orig.hasUserFloat("pfIso03") ? orig.userFloat("pfIso03") : -1.;

  }

  void convert(const std::vector<pat::Muon> & orig, std::vector<alp::Lepton> & dest) {
    dest.clear();
    for (const auto & e : orig) {
      dest.emplace_back();
      convert(e,dest.back());
    }
  }

  void convert(const pat::MET & orig, alp::Candidate & dest) {

    // four-momenta
    dest.p4_ = orig.p4();

  }

  void convert(const reco::GenParticle & orig, alp::Candidate & dest) {

    // four-momenta
    dest.p4_ = orig.p4();

  }

  void convert(const std::vector<reco::GenParticle> & orig, std::vector<alp::Candidate> & dest) {
    dest.clear();
    for (const auto & e : orig) {
      dest.emplace_back();
      convert(e,dest.back());
    }
  }

}
  

