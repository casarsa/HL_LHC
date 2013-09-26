// Class for the sector testing
// For more info, look at the header file

#include "sector_test.h"

sector_test::sector_test(std::string filename, std::string secfilename, 
			 std::string pattfilename, std::string outfile
			 , int nevt, bool dbg)
{  
  m_dbg    = dbg;
  evtIDmax = 0;

  if (pattfilename!="") 
  {
    sector_test::translateTuple(pattfilename,"rewritten.root",m_dbg); // Merging and reordering stage
    sector_test::initTuple(filename,"rewritten.root",outfile);
  }
  else
  {
    sector_test::initTuple(filename,pattfilename,outfile);
    evtIDmax = m_L1TT->GetEntries();
  }

  if (!sector_test::convert(secfilename)) return; // Don't go further if there is no sector file

  sector_test::do_test(nevt); // Launch the test loop over n events
}


/////////////////////////////////////////////////////////////////////////////////
//
// ==> sector_test::do_test(int nevt)
//
// Main method, where the efficiency calculations are made
//
/////////////////////////////////////////////////////////////////////////////////

void sector_test::do_test(int nevt)
{
  int id;

  const int m_nsec = m_sec_mult; // How many sectors are in the file
  const int m_nevt = evtIDmax;   // The max eventID in the sample

  int ndat = std::min(nevt,static_cast<int>(m_L1TT->GetEntries())); // How many events will we test

  cout << "Starting a test loop over " << ndat << " events..." << endl;
  cout << "... using " << m_nsec << " trigger sectors..." << endl;

  int is_sec_there[m_nsec];
  int ladder,module,layer;
  int n_per_lay[20];
  int n_per_lay_patt[20];
  int n_rods[6] = {16,24,34,48,62,76};
  int nhits_p;

  for (int j=0;j<500;++j) mult[j]=0;

  m_primaries.clear(); 

  std::vector<int> the_sectors;
  std::vector<int> prim_stubs;

  // We do a linking 
  // the older version of the PatternExtractor (before Sept. 20th 2013)) 

  // Loop over the events
  for (int i=0;i<ndat;++i)
  {    
    ntotpatt=0; 
    m_primaries.clear(); 

    m_L1TT->GetEntry(i);

    if (i%100000==0) 
      cout << "Processed " << i << "/" << ndat << endl;

    if (m_stub == 0) continue; // No stubs, don't go further

    if (m_dbg) m_evtid=i;

    if (m_evtid>=m_nevt) continue; // No PR output available

    evt = i;

    if (do_patt)
    {
      m_PATT->GetEntry(m_evtid);      
      evt = event_id;

      if (nb_patterns==-1) continue; // This event wasn't treated
    }    


    //
    // First, we make a loop on the stubs in order to check 
    // how many primary tracks we have
    //
    // The info is then stored in the m_primaries vector
    //

    bool already_there = 0;


    //    cout << i << " // " << m_stub << endl;

    for (int j=0;j<m_stub;++j)
    {  
      if (m_stub_tp[j]<0) continue; // Bad stub  

      // Basic primary selection (pt and d0 cuts)
      if (sqrt(m_stub_pxGEN[j]*m_stub_pxGEN[j]+m_stub_pyGEN[j]*m_stub_pyGEN[j])<0.1) continue;
      if (sqrt(m_stub_X0[j]*m_stub_X0[j]+m_stub_Y0[j]*m_stub_Y0[j])>2.) continue; 

      already_there = false;

      for (unsigned int k=0;k<m_primaries.size();++k) // Check if it's already been found
      {  
	if (m_primaries.at(k).at(0)!=m_stub_tp[j]) continue;

	already_there = true;
	m_primaries.at(k).push_back(j); // If yes, just put the stub index
	break;
      }

      if (already_there == true) continue;   

      // Here we have a new primary, we create a new entry

      prim_stubs.clear();
      prim_stubs.push_back(m_stub_tp[j]); // The TP id 
      prim_stubs.push_back(j); // The first stub ID

      m_primaries.push_back(prim_stubs);
    }

    if (m_primaries.size()>1)
      std::cout << "Found " << m_primaries.size() 
		<< " primary particles in the event" << i << std::endl; 


    //
    // Then, in the second loop, we test all the primaries 
    // in order to check if they have matched a pattern
    //


    int idx = 0;

    for (unsigned int k=0;k<m_primaries.size();++k)
    {
      if (m_primaries.at(k).size()<4) continue; // Less then 4 stubs, give up this one

      for (int j=0;j<20;++j) n_per_lay[j]=0;
      for (int j=0;j<20;++j) n_per_lay_patt[j]=0;

      for (int j=0;j<m_nsec;++j)
      {
	is_sec_there[j]=0;
	mult[j]=0;
      }

      nhits=0;
      nsec=0;
      eta=0.;
      pt=0.;
      phi=0.;
      d0=-1.;
      npatt=-1;

      // Here we make a loop to compute the value of NHits

      for (unsigned int j=1;j<m_primaries.at(k).size();++j)
      {
	idx    = m_primaries.at(k).at(j);

	// First of all we compute the ID of the stub's module
	layer  = m_stub_layer[idx]; 
	ladder = m_stub_ladder[idx]; 
	module = m_stub_module[idx]; 

	///////
	// This hack is temporary and is due to a numbering problem in the TkLayout tool
	if (layer<=10) ladder = (ladder+n_rods[layer-5]/4)%(n_rods[layer-5]);
	///////

	id = 10000*layer+100*ladder+module; // Get the module ID
      
	if (m_modules.at(id).size()>1)
	{
	  for (unsigned int kk=1;kk<m_modules.at(id).size();++kk) // In which sector the module is
	  {
	    ++mult[m_modules.at(id).at(kk)];
	    ++is_sec_there[m_modules.at(id).at(kk)]; 
	  }
	}

	++n_per_lay[layer-5];
	pt  = sqrt(m_stub_pxGEN[idx]*m_stub_pxGEN[idx]+m_stub_pyGEN[idx]*m_stub_pyGEN[idx]);
	eta = m_stub_etaGEN[idx];
	phi = atan2(m_stub_pyGEN[idx],m_stub_pxGEN[idx]);
	d0  = sqrt(m_stub_X0[idx]*m_stub_X0[idx]+m_stub_Y0[idx]*m_stub_Y0[idx]);
	pdg = m_stub_pdg[idx];
      }
    
      // For the sector efficiency we proceed as follow
      // We check if the track has let at least 4 stubs in 
      // 4 different layer/disk (nhits>=4). If yes, we compute 
      // the number of sectors containing at least 4 of those 
      // stubs (nsec)

      // First we get the number of different layers/disks hit by the primary
      for (int kk=0;kk<20;++kk)
      {
	if (n_per_lay[kk]!=0) ++nhits;
	nplay[kk]=n_per_lay[kk];
      }

      // Then we get the number of sectors containing more than 4 primary hits 
      for (int kk=0;kk<m_nsec;++kk)
      {
	if (is_sec_there[kk]>=4) ++nsec; 
      }

      // Finally we do the pattern loop
      
      if (do_patt)
      {
	ntotpatt = nb_patterns; // The total number of patterns in the sector/event
	npatt    = 0;           // The patterns containing at least 4 prim hits
	
	if (nb_patterns>0)
	{	  
	  for(int kk=0;kk<nb_patterns;kk++)
	  {
	    for (int j=0;j<20;++j) n_per_lay_patt[j]=0;
	   
	    // First we count the number of prim stubs in the 
	    // pattern layers/disks
	
	    if (m_links.at(kk).size()==0) continue;

	    for(unsigned int m=0;m<m_links.at(kk).size();m++)
	    {
	      if (m_links.at(kk).at(m)>=m_stub || m_links.at(kk).at(m)<0)
	      {
		cout << " !!! Reordering problem !!! " << m_stub << endl;
		cout << m_links.at(kk).at(m) << " / " << m_stub << endl;
		cout << m_evtid << " // " << event_id << endl;
	      }
	      
	      if (m_stub_tp[m_links.at(kk).at(m)]==m_primaries.at(k).at(0))
		n_per_lay_patt[m_stub_layer[m_links.at(kk).at(m)]-5]++;

	      	     
	    }
	   
	    // First we get the number of different layers/disks hit by the primary
	    // in the pattern

	    nhits_p=0; 
      
	    for (int kk=0;kk<20;++kk)
	    {
	      if (n_per_lay_patt[kk]!=0) ++nhits_p;
	    }
	
	    if (nhits_p>=4) ++npatt; // More than 4, the pattern is good
	  }
	  	  
	}	
      }
      

      m_efftree->Fill();
    }
  }
 
  m_outfile->Write();
  m_outfile->Close();
}


/////////////////////////////////////////////////////////////////////////////////
//
// ==> sector_test::initTuple(std::string test,std::string patt,std::string out)
//
// This method opens and creates the differe rootuples involved
//
/////////////////////////////////////////////////////////////////////////////////


void sector_test::initTuple(std::string test,std::string patt,std::string out)
{
  m_L1TT   = new TChain("L1TrackTrigger"); 

  // Input data file (you can use rfio of xrood address here)
  m_L1TT->Add(test.c_str());
  
  // Example of an xrootd data file
  //m_L1TT->Add("root://lyogrid06.in2p3.fr//dpm/in2p3.fr/home/cms/data/store/user/sviret/SLHC/GEN/612_SLHC6_PU4T_thresh2/PILEUP4T_140_02.root");   
  pm_stub_layer=&m_stub_layer;
  pm_stub_ladder=&m_stub_ladder;
  pm_stub_module=&m_stub_module;
  pm_stub_pxGEN=&m_stub_pxGEN;
  pm_stub_pyGEN=&m_stub_pyGEN;
  pm_stub_etaGEN=&m_stub_etaGEN;
  pm_stub_tp=&m_stub_tp;
  pm_stub_pdg=&m_stub_pdg;
  pm_stub_X0=&m_stub_X0;
  pm_stub_Y0=&m_stub_Y0;

  m_L1TT->SetBranchAddress("evt",            &m_evtid); 
  m_L1TT->SetBranchAddress("STUB_n",         &m_stub);
  m_L1TT->SetBranchAddress("STUB_layer",     &pm_stub_layer);
  m_L1TT->SetBranchAddress("STUB_ladder",    &pm_stub_ladder);
  m_L1TT->SetBranchAddress("STUB_module",    &pm_stub_module);
  m_L1TT->SetBranchAddress("STUB_pxGEN",     &pm_stub_pxGEN);
  m_L1TT->SetBranchAddress("STUB_pyGEN",     &pm_stub_pyGEN);
  m_L1TT->SetBranchAddress("STUB_X0",        &pm_stub_X0);
  m_L1TT->SetBranchAddress("STUB_Y0",        &pm_stub_Y0);
  m_L1TT->SetBranchAddress("STUB_etaGEN",    &pm_stub_etaGEN);
  m_L1TT->SetBranchAddress("STUB_tp",        &pm_stub_tp);
  m_L1TT->SetBranchAddress("STUB_pdgID",     &pm_stub_pdg);
  

  // Output file definition (see the header)

  m_outfile = new TFile(out.c_str(),"recreate");
  m_efftree = new TTree("SectorEff","");

  m_efftree->Branch("event",      &evt,     "event/I"); 
  m_efftree->Branch("pdgID",      &pdg,     "pdgID/I"); 
  m_efftree->Branch("nsec",       &nsec,    "nsec/I"); 
  m_efftree->Branch("nhits",      &nhits,   "nhits/I"); 
  m_efftree->Branch("npatt",      &npatt,   "npatt/I"); 
  m_efftree->Branch("tpatt",      &ntotpatt,"tpatt/I"); 
  m_efftree->Branch("nplay",      &nplay,   "nplay[20]/I"); 
  m_efftree->Branch("pt",         &pt,      "pt/F"); 
  m_efftree->Branch("eta",        &eta,     "eta/F"); 
  m_efftree->Branch("phi",        &phi,     "phi/F"); 
  m_efftree->Branch("d0",         &d0,      "d0/F"); 
  m_efftree->Branch("mult",       &mult,    "mult[500]/I"); 


  // Pattern file information only if available

  do_patt=false;

  if (patt!="")
  {
    do_patt = true;
    m_pattfile  = TFile::Open(patt.c_str());

    m_PATT    = (TTree*)m_pattfile->Get("L1PatternReco");
    
    pm_links=&m_links;
    pm_secid=&m_secid;
    
    m_PATT->SetBranchAddress("evt",            &event_id); 
    m_PATT->SetBranchAddress("PATT_n",         &nb_patterns);
    m_PATT->SetBranchAddress("PATT_links",     &pm_links);
    m_PATT->SetBranchAddress("PATT_secID",     &pm_secid);
  }
}


/////////////////////////////////////////////////////////////////////////////////
//
// ==> sector_test::translateTuple(std::string pattin,std::string pattout)
//
// This method performs the merging and reordering stage.
//
// The input file (pattin) contains a rough merging of all the PR output files from CMSSW jobs.
// It is obtained via hadd, and therefore has still to be reordered.
//
// The method loops over all pattin entries in order to get the number of different events 
// contained into it, and then group the results for all these event.
//
// The output file pattout contains the same tree than pattin, but with only one entry per event.
// This entry contains all the pattern matched for this event, for all the sectors (the sector ID info 
// is kept also)
//
/////////////////////////////////////////////////////////////////////////////////

void sector_test::translateTuple(std::string pattin,std::string pattout,bool m_dbg)
{
  std::vector< std::vector<int> > m_links;
  std::vector<int> m_secid;
  std::vector< std::vector<int> > *pm_links = new std::vector< std::vector<int> >;
  std::vector<int> *pm_secid = new std::vector<int> ;

  pm_links=&m_links;
  pm_secid=&m_secid;

  std::vector<int> links;

  int evtID;
  int m_patt;

  const int MAX_NB_PATTERNS = 1500;
  const int MAX_NB_HITS     = 100;

  int   pattern_sector_id[MAX_NB_PATTERNS];  
  int   nbHitPerPattern[MAX_NB_PATTERNS];
  int   hit_idx[MAX_NB_PATTERNS*MAX_NB_HITS];

  if (m_dbg) // The file from a standalone PR (use for debugging)
  {
    data = new TChain("Patterns");

    data->SetBranchAddress("eventID",             &evtID);         
    data->SetBranchAddress("nbPatterns",          &m_patt);
    data->SetBranchAddress("sectorID",            pattern_sector_id); 
    data->SetBranchAddress("nbStubs",             nbHitPerPattern);   
    data->SetBranchAddress("stub_idx",            hit_idx);      
  }
  else // The classic PR output (using CMSSW)
  {
    data = new TChain("L1PatternReco");
    
    data->SetBranchAddress("evt",            &evtID); // Simple evt number or event ID
    data->SetBranchAddress("PATT_n",         &m_patt);
    data->SetBranchAddress("PATT_links",     &pm_links);
    data->SetBranchAddress("PATT_secID",     &pm_secid);
  }

  data->Add(pattin.c_str());

  std::vector< std::vector<int> > *f_links = new std::vector< std::vector<int> >;
  std::vector<int> *f_secid = new std::vector<int> ;

  int f_evtID;
  int f_patt;

  TFile hfile(pattout.c_str(),"RECREATE","");

  TTree *m_tree_L1PatternReco = new TTree("L1PatternReco","L1PatternReco Analysis info");  
  
  /// Branches definition

  m_tree_L1PatternReco->Branch("evt",            &f_evtID); // Simple evt number or event ID
  m_tree_L1PatternReco->Branch("PATT_n",         &f_patt);
  m_tree_L1PatternReco->Branch("PATT_links",     &f_links);
  m_tree_L1PatternReco->Branch("PATT_secID",     &f_secid);

  int ndat = data->GetEntries();

  evtIDmax = -1;

  std::vector<int> indexes;
  std::vector< std::vector<int> > indexes_ordered;

  indexes.clear();

  // First loop is over all pattin entries
    
  for(int i=0;i<ndat;i++)
  {    
    data->GetEntry(i);

    if (evtID>evtIDmax) evtIDmax=evtID;

    indexes.push_back(evtID);
  }
 

  // We have the maximum event number contained in pattin. We create index table of this size
  //


  evtIDmax+=1;

  indexes_ordered.resize(evtIDmax);

  for(int i=0;i<evtIDmax;i++) indexes_ordered[i].clear();
  for(int i=0;i<ndat;i++)     indexes_ordered[indexes[i]].push_back(i);

  // index_ordered contains, for each evtid, a vector containing 
  // the related entries of pattin related to this evtid 

  // Then we fill the new rootuple 
  //

  int hitIndex;

  for(int i=0;i<evtIDmax;i++)
  { 
    if (i%100000==0)
      cout << "Reordering event ID " << i << endl;
  
    //f_evtID = i+1;
    f_evtID = i;
    f_patt = 0;
    f_links->clear();
    f_secid->clear();
    hitIndex = 0; 

    if (indexes_ordered[i].size()!=0) 
    {
      for(unsigned int j=0;j<indexes_ordered[i].size();j++)
      {    
	data->GetEntry(indexes_ordered[i].at(j));
      
	if (evtID!=i) continue; 
	if (m_patt==0) continue;

	f_patt += m_patt;

	for(int k=0;k<m_patt;k++)
	{ 
	  if (m_dbg) // Debug mode
	  {
	    links.clear();

	    for(int m=0;m<nbHitPerPattern[k];m++)
	    {
	      links.push_back(hit_idx[hitIndex]);
	      hitIndex++;
	    }

	    f_links->push_back(links);
	    f_secid->push_back(pattern_sector_id[k]);
	  }
	  else
	  {
	    f_links->push_back(m_links.at(k));
	    f_secid->push_back(m_secid.at(k));
	  }
	}
      }
    }
    else // This event is not in the PR output, skip it
    {
      f_patt = -1;
    }

    m_tree_L1PatternReco->Fill();
  }

  hfile.Write();
  hfile.Close();
}


/////////////////////////////////////////////////////////////////////////////////
//
// ==> sector_test::convert(std::string sectorfilename) 
//
// Here we retrieve info from the TKLayout CSV file containing the sector definition
//
// This file contains, for each sector, the ids of the modules contained in the sector
//
// The role of this method is to create the opposite, ie a vector containing, for every module the list of sectors belonging to it
//
/////////////////////////////////////////////////////////////////////////////////

bool sector_test::convert(std::string sectorfilename) 
{
  m_sec_mult = 0;

  std::vector<int> module;

  m_modules.clear();

  for (int i=0;i<230000;++i)
  {
    module.clear();
    module.push_back(-1);
    m_modules.push_back(module);
  }

  std::string STRING;
  std::ifstream in(sectorfilename.c_str());
  if (!in) 
  {
    std::cout << "Please provide a valid csv sector filename" << std::endl; 
    return false;
  }    
  
  int npar = 0;

  while (!in.eof()) 
  {
    ++m_sec_mult;
    if (m_sec_mult<=2) continue;

    getline(in,STRING);
    std::istringstream ss(STRING);
    npar = 0;
    while (ss)
    {
      std::string s;
      if (!getline( ss, s, ',' )) break;

      ++npar;
      if (npar<=2) continue;

      m_modules.at(atoi(s.c_str())).push_back(m_sec_mult-4);
    }
  }

  in.close();

  m_sec_mult -= 5;

  return true;
}
