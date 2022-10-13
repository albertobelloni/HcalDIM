import FWCore.ParameterSet.Config as cms

process = cms.Process("HexDump")

process.load("FWCore.MessageService.MessageLogger_cfi")

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(10) )

process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring('file:step3.root'))


hcalhexinspector = cms.EDAnalyzer(
    'HcalHexInspector',
    feds = cms.untracked.vint32(712,731), # things seem not to work w/o explicit list of FEDs...
    dumpBinary = cms.untracked.bool(True),
    dumpLevel = cms.untracked.int32(4),
    CheckQIEData = cms.untracked.bool(True),
    HistoQIEData = cms.untracked.bool(True),
    debug = cms.untracked.bool(True)
)

process.hexdump = hcalhexinspector

process.p = cms.Path(process.hexdump)
