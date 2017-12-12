import os,sys
import ROOT as rt

rt.gStyle.SetOptStat(0)

inputfile = sys.argv[1]

rfile = rt.TFile(inputfile)

tree = rfile.Get("flashtimes")

nentries = tree.GetEntries()

highx_cut = 100.0
lowx_cut  = highx_cut*1.5
peratio_cut = 1000.0
use_scan = True
golden_cut = True
use_containment = False
use_cosmicratio = True

# test cut variables
neff_truepos = 0   # fraction of vertex-matched rois that pass
neff_falsepos = 0  # fraction of non-vertex-matched rois that pass
neff_event = 0     # events with vertex-matched roi that has one pass flash cut
nnon_matched = 0
nmatched = 0
nevents = 0
nevents_wmatch = 0

print "Number of entries in tree: ",nentries

ientry = 0
bytesread = tree.GetEntry(ientry)

plot_types = ["vtxmatched","notvtxmatched","data"]


# DEFINE HISTOGRAMS

# roi-x
hroix = {}
for ptype in plot_types[:2]:
    hroix[ptype] = rt.TH1D("hroix_"+ptype,";x (cm); AU",50,-25,275.0)

# fractional difference from true and hypothesis flash
hpediff = {}
for ptype in plot_types[:2]:
    hpediff[ptype] = rt.TH1D("hpediff_"+ptype,";(PE_{hypo}-PE_{data})/PE_{data}; AU",400,-20.0,20.0)

# hpediff versus pos[0]
hpediff_v_pos = {}
hpediff_v_pos["vtxmatched"] = rt.TH2D("hpediff_v_x_vtxmatched","Vertex-matched ROIs;x (cm);(PE_{hypo}-PE_{data})/PE_{data}",30,-25,275,75,-10.0,20.0)
hpediff_v_pos["notvtxmatched"] = rt.TH2D("hpediff_v_x_notvtxmatched","Not vertex-matched ROIs;x (cm);(PE_{hypo}-PE_{data})/PE_{data}",30,-25,275,75,-10.0,20.0)

# chi-squared
hchi2 = {}
hchi2_v_x = {}
for ptype in plot_types[:2]:
    hchi2[ptype]     = rt.TH1D("hchi2_"+ptype,";#chi^{2}; counts",50,0,1000.0)
    hchi2_v_x[ptype] = rt.TH2D("hchi2_v_x_"+ptype,";x (cm);#chi^{2}",30,-25,275,50,0,1000.0)

# scanned chi-squared
hscan = {}
hscan_v_x = {}
for ptype in plot_types[:2]:
    hscan[ptype]     = rt.TH1D("hscan_"+ptype,";#chi^{2}; counts",50,0,1000.0)
    hscan_v_x[ptype] = rt.TH2D("hscan_v_x_"+ptype,";x (cm);#chi^{2}",30,-25,275,50,0,1000.0)

# gaus ll
hgausll = {}
hgausll_v_x = {}
for ptype in plot_types[:2]:
    hgausll[ptype]     = rt.TH1D("hgausll_"+ptype,";#chi^{2}; counts",150,0,15.0)
    hgausll_v_x[ptype] = rt.TH2D("hgausll_v_x_"+ptype,";x (cm);#chi^{2}",30,-25,275,50,0,10.0)
    
# event efficiency
heff_pass = rt.TH1D("heff_pass",";Enu;efficiency",10,0,1000)
heff_tot = rt.TH1D("heff_tot",";Enu;efficiency",10,0,1000)

# num ROI
hnroi_pass = rt.TH1D("hnroi_pass",";Num ROI;",30,0,30)
hnroi_tot  = rt.TH1D("hnroi_tot",";Num ROI;",30,0,30)


while bytesread>0:
    print "entry ",ientry

    #fraction difference between data and hypothesis flash
    pefrac_true = {}
    pefrac_false = {}    
    for i in range(0,tree.totpe_data.size()):
        totpe_data = tree.totpe_data[i]
        if totpe_data==0:
            continue

        # PE DIFF
        for j in range(0,tree.totpe_hypo_trueflashes.size()):
            pediff = tree.totpe_hypo_trueflashes[j]-totpe_data
            pefracdiff = pediff/totpe_data
            hpediff["vtxmatched"].Fill( pefracdiff )
            hpediff_v_pos["vtxmatched"].Fill( tree.pos[0], pefracdiff )
            if j not in pefrac_true:
                pefrac_true[j] = pefracdiff
            else:
                if fabs(pefrac_true[j])>fabs(pefracdiff):
                    pefrac_true[j] = pefracdiff
        for j in range(0,tree.totpe_hypo_falseflashes.size()):
            pediff = tree.totpe_hypo_falseflashes[j]-totpe_data
            pefracdiff = pediff/totpe_data
            hpediff["notvtxmatched"].Fill( pefracdiff )
            hpediff_v_pos["notvtxmatched"].Fill(tree.pos[0],  pefracdiff )
            if j not in pefrac_false:
                pefrac_false[j] = pefracdiff            
            else:
                if fabs(pefrac_false[j])>fabs(pefracdiff):
                    pefrac_false[j] = pefracdiff

    # FIXED CHI2
    for j in range(0,tree.smallest_chi2_trueflashes.size()):
        hchi2["vtxmatched"].Fill( tree.smallest_chi2_trueflashes[j] )
        hchi2_v_x["vtxmatched"].Fill( tree.pos[0], tree.smallest_chi2_trueflashes[j] )
    for j in range(0,tree.smallest_chi2_falseflashes.size()):
        hchi2["notvtxmatched"].Fill( tree.smallest_chi2_falseflashes[j] )
        hchi2_v_x["notvtxmatched"].Fill( tree.pos[0], tree.smallest_chi2_falseflashes[j] )

    # Gaus LL
    for j in range(0,tree.smallest_gausll_trueflashes.size()):
        hgausll["vtxmatched"].Fill( tree.smallest_gausll_trueflashes[j] )
        hgausll_v_x["vtxmatched"].Fill( tree.pos[0], tree.smallest_gausll_trueflashes[j] )
    for j in range(0,tree.smallest_gausll_falseflashes.size()):
        hgausll["notvtxmatched"].Fill( tree.smallest_gausll_falseflashes[j] )
        hgausll_v_x["notvtxmatched"].Fill( tree.pos[0], tree.smallest_gausll_falseflashes[j] )

    # Gaus LL
    for j in range(0,tree.smallest_scan_trueflashes.size()):
        hscan["vtxmatched"].Fill( tree.smallest_scan_trueflashes[j] )
        hscan_v_x["vtxmatched"].Fill( tree.pos[0], tree.smallest_scan_trueflashes[j] )
    for j in range(0,tree.smallest_scan_falseflashes.size()):
        hscan["notvtxmatched"].Fill( tree.smallest_scan_falseflashes[j] )
        hscan_v_x["notvtxmatched"].Fill( tree.pos[0], tree.smallest_scan_falseflashes[j] )

    # determine cut
    hastruepass = False    
    src_trueflashes = tree.smallest_chi2_trueflashes
    src_falseflashes = tree.smallest_chi2_falseflashes    
    if use_scan:
        src_trueflashes = tree.smallest_scan_trueflashes
        src_falseflashes = tree.smallest_scan_falseflashes

    nroi = 0
    nroi_tot = 0
    for j in range(0,src_trueflashes.size()):
        # nonflash cuts
        nonflashcut = True
        nroi_tot+=1
        if use_containment and tree.containment_trueflashes[j]==0:
            nonflashcut = False
        if use_cosmicratio and tree.cosmicratio_trueflashes[j]==0:
            nonflashcut = False
        
        if tree.pos[0]<100 and src_trueflashes[j]<lowx_cut and nonflashcut:
            neff_truepos+=1
            hastruepass = True
            nroi += 1
        elif tree.pos[0]>100 and src_trueflashes[j]<highx_cut and nonflashcut:
            neff_truepos+=1
            hastruepass = True
            nroi += 1
        nmatched += 1
    for j in range(0,src_falseflashes.size()):
        nonflashcut = True
        nroi_tot+=1        
        if use_containment and tree.containment_falseflashes[j]==0:
            nonflashcut = False
        if use_cosmicratio and tree.cosmicratio_falseflashes[j]==0:
            nonflashcut = False        
        if tree.pos[0]<100 and src_falseflashes[j]<lowx_cut and nonflashcut:
            neff_falsepos+=1
            nroi += 1
        elif tree.pos[0]>100 and src_falseflashes[j]<highx_cut and nonflashcut:
            neff_falsepos+=1
            nroi += 1
        nnon_matched += 1

    if not golden_cut:
        nevents += 1
        if hastruepass:
            neff_event += 1
            nevents_wmatch += 1
        hnroi_pass.Fill( nroi )
        hnroi_tot.Fill( nroi_tot )        
    else:
        if tree.EnuGeV>0.2 and tree.dwall>10.0:
            nevents += 1
            if hastruepass:
                neff_event += 1
                nevents_wmatch += 1
            hnroi_pass.Fill( nroi )
            hnroi_tot.Fill( nroi_tot )

    if hastruepass:
        heff_pass.Fill( tree.EnuGeV*1000.0 )
    heff_tot.Fill( tree.EnuGeV*1000.0 )
        
    # ROI-x
    for j in range(0,src_trueflashes.size()):
       hroix["vtxmatched"].Fill( tree.pos[0] )
    for j in range(0,src_falseflashes.size()):
       hroix["notvtxmatched"].Fill( tree.pos[0] )
        
    ientry += 1
    bytesread = tree.GetEntry(ientry)

# CANVASES

# pediff
cpediff = rt.TCanvas("pediff","",800,600)
hpediff["vtxmatched"].SetLineColor(rt.kRed)
hpediff["notvtxmatched"].Draw()
hpediff["vtxmatched"].Draw("same")
cpediff.Draw()
cpediff.SetLogy(1)
cpediff.Update()

# pediff v pos
cpediff_v_x = rt.TCanvas("pediff_v_pos","PE diff versus x",1400,600)
cpediff_v_x.Divide(2,1)
cpediff_v_x.cd(1)
hpediff_v_pos["vtxmatched"].Draw("COLZ")
cpediff_v_x.cd(2)
hpediff_v_pos["notvtxmatched"].Draw("COLZ")
cpediff_v_x.Draw()
cpediff_v_x.Update()

# Chi2
cchi2 = rt.TCanvas("cchi2","FIXED CHI2",800,600)
cchi2.Draw()
hchi2["vtxmatched"].SetLineColor(rt.kRed)
hchi2["notvtxmatched"].SetMinimum(0)
hchi2["notvtxmatched"].Draw()
hchi2["vtxmatched"].Draw("same")
cchi2.Update()

# chi2 v x
cchi2_v_x = rt.TCanvas("chi2_v_pos","FIXED Chi-squared versus x",1400,600)
cchi2_v_x.Divide(2,1)
cchi2_v_x.cd(1)
hchi2_v_x["vtxmatched"].Draw("COLZ")
cchi2_v_x.cd(2)
hchi2_v_x["notvtxmatched"].Draw("COLZ")
cchi2_v_x.Draw()
cchi2_v_x.Update()

# Gausll
cgausll = rt.TCanvas("cgausll","GAUS LL",800,600)
cgausll.Draw()
hgausll["vtxmatched"].SetLineColor(rt.kRed)
hgausll["notvtxmatched"].SetMinimum(0)
hgausll["notvtxmatched"].Draw()
hgausll["vtxmatched"].Draw("same")
cgausll.Update()

# Scan
cscan = rt.TCanvas("cscan","SCAN CHI2",800,600)
cscan.Draw()
hscan["vtxmatched"].SetLineColor(rt.kRed)
hscan["notvtxmatched"].SetMinimum(0)
hscan["notvtxmatched"].Draw()
hscan["vtxmatched"].Draw("same")
cscan.Update()

# gausll v x
cgausll_v_x = rt.TCanvas("gausll_v_pos","Gaus LL versus x",1400,600)
cgausll_v_x.Divide(2,1)
cgausll_v_x.cd(1)
hgausll_v_x["vtxmatched"].Draw("COLZ")
cgausll_v_x.cd(2)
hgausll_v_x["notvtxmatched"].Draw("COLZ")
cgausll_v_x.Draw()
cgausll_v_x.Update()

# scan v x
cscan_v_x = rt.TCanvas("scan_v_pos","SCAN Chi-squared versus x",1400,600)
cscan_v_x.Divide(2,1)
cscan_v_x.cd(1)
hscan_v_x["vtxmatched"].Draw("COLZ")
cscan_v_x.cd(2)
hscan_v_x["notvtxmatched"].Draw("COLZ")
cscan_v_x.Draw()
cscan_v_x.Update()

# ROI-x
croix = rt.TCanvas("croix","",800,600)
hroix["vtxmatched"].SetLineColor(rt.kRed)
hroix["notvtxmatched"].SetMinimum(0)
hroix["notvtxmatched"].Draw()
hroix["vtxmatched"].Draw("same")
croix.Draw()
croix.Update()

# Efficiency versus EnuGeV
ceff = rt.TCanvas("ceff","Efficiency vs. Enu", 800, 600 )
heff_pass.Divide(heff_tot)
heff_pass.Draw()
ceff.Draw()

# Efficiency versus EnuGeV
cnroi = rt.TCanvas("cnroi","Num ROI per event", 1400, 600 )
cnroi.Divide(2,1)
cnroi.cd(1)
hnroi_pass.Draw()
cnroi.cd(2)
hnroi_tot.Draw()
cnroi.Draw()


# Efficiency numbers
nevts_has_matched = tree.GetEntries("num_vtxmatched_tracks>0")
nevts_has_matchedbbox = tree.GetEntries("num_true_bbox>0")

print "High-x chi-2 cut: ",highx_cut
print "Low-x chi-2 cut: ",lowx_cut

print "Number of Events: ",nevents
print "Number of Events w/ vertex-matched tracks: ",nevts_has_matched," ",float(nevts_has_matched)/float(nevents)
print "Number of Events w/ vertex-matched ROIs: ",nevts_has_matchedbbox," ",float(nevts_has_matchedbbox)/float(nevents)
print "Efficiency for Vertex-matched ROIs: ",float(neff_truepos)/float(nmatched)
print "Fraction of non-vertex-matched ROIs that PASS (false positives): ",float(neff_falsepos)/float(nnon_matched)
print "Fraction of non-vertex-matched ROIs that REJECTED (true negatives): ",1.0-float(neff_falsepos)/float(nnon_matched)
print "Total Fraction of events with ROI match: ",float(nevents_wmatch)/float(nevents)
print "Fraction of events with passing vertex-matched ROI: ",float(neff_event)/float(nevents)
print "False positives per event: ",float(neff_falsepos)/float(nevents)
print "True/False positive ratio: ",float(neff_truepos)/float(neff_falsepos)

raw_input()