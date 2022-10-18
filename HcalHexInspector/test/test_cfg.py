import FWCore.ParameterSet.Config as cms

process = cms.Process("Demo")

process.load("FWCore.MessageService.MessageLogger_cfi")

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(10) )

process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring('root://xrootd-cms.infn.it//store/data/Run2022D/Cosmics/RAW-RECO/CosmicSP-PromptReco-v3/000/358/831/00000/7edf4505-c032-4a3f-9382-4f1832b6950e.root'))
    #fileNames = cms.untracked.vstring('file:step3.root'))

process.demo = cms.EDAnalyzer('Test')


process.p = cms.Path(process.demo)
