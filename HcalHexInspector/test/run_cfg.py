import FWCore.ParameterSet.Config as cms

process = cms.Process("HexDump")

process.load("FWCore.MessageService.MessageLogger_cfi")

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(10) )

process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring('file:step3.root'))


hcalhexinspector = cms.EDAnalyzer(
    'HcalHexInspector',
    dumpBinary = cms.untracked.bool(True),
    debug = cms.untracked.bool(True)
)

process.hexdump = hcalhexinspector

process.p = cms.Path(process.hexdump)
