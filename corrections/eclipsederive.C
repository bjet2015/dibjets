#include "../helpers/plotting.h"
#include "../helpers/config.h"
#include "TProfile.h"

//to derive from NS:
// nt->Draw("jtpt2>>h(140,40,180)","weight*(jtpt1>100 && jtpt2>40 && dphi21<1.05 && bin<20 && subid2!=0 && pthat>65)")
// nt->Draw("jtpt2>>h(140,40,180)","weight*(jtpt1>100 && jtpt2>40 && dphi21<1.05 && bin<20 && pthat>65)")
// auto v = h->GetIntegral()
// vector<double>vx;
// for (int i=40;i<180;i++) vx.push_back(i)
// auto g = new TGraph(140, &vx[0],v)
// g->Draw()
// TCanvas cc
// g->Draw()
// 
// auto f = new TF1("f","exp(-[0]*exp(-[1]*x))");
// f->SetParameters(100,0.1);
// g->Fit(f);

vector<int> binbounds = {0,5,10,15,20,30,40,50,60,200};
vector<float> binmean;
vector<TProfile *> profs;
vector<TF1 *>fs;

float shift = 0;

void derivefromprofile()
{

  auto file = config.getfile_djt("mcPbqcd");
  auto nt = (TTree *)file->Get("nt");

  for (unsigned i=1;i<binbounds.size();i++) {
    int b1 = binbounds[i-1];
    int b2 = binbounds[i];
    auto p = new TProfile(Form("p%d%d",b1,b2),Form("prof"),100,0,200);

    nt->Project(p->GetName(),"(subid2 == 0 && refpt2 > 20):jtptSignal2",Form("weight*(jtpt1>100&&bin>=%d && bin<%d)",b1,b2));
    profs.push_back(p);

    auto f = new TF1(Form("f%d%d",b1,b2),"exp(-[0]*exp(-[1]*x))");
    f->SetParameters(100,0.1);
    p->Fit(f);
    fs.push_back(f);

    float median = -1/f->GetParameter(1)*log(-1/f->GetParameter(0)*log(0.5));

    auto c = getc();
      TLatex *Tl = new TLatex();
    p->SetMinimum(0);p->SetMaximum(1);
    p->Draw();
    f->Draw("same");
    Tl->DrawLatexNDC(0.6,0.4,Form("PbPb bin %d-%d",b1,b2));
    Tl->DrawLatexNDC(0.6,0.35,Form("median = %.2f",median));

    TLine *l1 = new TLine(median,0,median, f->Eval(median));
    l1->Draw();
    TLine *l2 = new TLine(0,0.5,median, f->Eval(median));
    l2->Draw();

    SavePlots(c,Form("fit%d%d",b1,b2));
  }

}

float pt = 40;
vector<float> prob;
vector<float> meanb;

void derivefromNS(bool data = false)
{

  auto file = config.getfile_djt(data ? "dtPbj60" : "mcPbqcd");
  auto nt = (TTree *)file->Get("nt");

  for (unsigned i=1;i<binbounds.size();i++) {
    int b1 = binbounds[i-1];
    int b2 = binbounds[i];
    seth(71,38,180);
    auto h = geth(Form("h%d%d",b1,b2)); //allowing one more bin for overflow
    seth(b2-b1,b1,b2);
    auto hb = geth(Form("hb%d%d",b1,b2));

   
    TString mcappendix = data ? "" : "&& pthat>50";
   
    nt->Project(h->GetName(),Form("jtpt2+%f",shift), Form("weight*(jtpt1>100&&bin>=%d && bin<%d && dphi21<1.05 %s)",b1,b2,mcappendix.Data()));
    nt->Project(hb->GetName(),"bin", Form("weight*(jtpt1>100&&bin>=%d && bin<%d && dphi21<1.05 %s)",b1,b2,mcappendix.Data()));

    h->SetBinContent(1,h->GetBinContent(0)+h->GetBinContent(1));

    auto g = getCDFgraph(h);
    g->GetXaxis()->SetTitle("p_{T,2} threshold [GeV]");
    g->GetYaxis()->SetTitle("found fraction");

    auto gtemp = getCDFgraph(h);
    meanb.push_back(hb->GetMean());
    prob.push_back(gtemp->Eval(pt));

    auto f = new TF1(Form("f%d%d",b1,b2),"exp(-[0]*exp(-[1]*x))",40,180);
    f->SetLineColor(kRed);
    f->SetLineWidth(2);
    f->SetParameters(100,0.1);
    g->Fit(f,"RM");
    fs.push_back(f);
    binmean.push_back(hb->GetMean());

    float median = -1/f->GetParameter(1)*log(-1/f->GetParameter(0)*log(0.5));

    Draw({h});


    auto c = getc();
          TLatex *Tl = new TLatex();
    g->SetMinimum(0);g->SetMaximum(1);
    g->Draw("AP");
    f->Draw("same");
    Tl->DrawLatexNDC(0.6,0.55,"y=e^{-a e^{-b x} }");
    Tl->DrawLatexNDC(0.6,0.50,Form("a = %.1f",f->GetParameter(0)));
    Tl->DrawLatexNDC(0.6,0.45,Form("b = %.2f",f->GetParameter(1)));
    Tl->DrawLatexNDC(0.6,0.4,Form("PbPb bin %d-%d",b1,b2));
    Tl->DrawLatexNDC(0.6,0.35,Form("median = %.2f",median));

    TLine *l1 = new TLine(median,0,median, f->Eval(median));
    l1->Draw();
    TLine *l2 = new TLine(0,0.5,median, f->Eval(median));
    l2->Draw();



    SavePlots(c,Form("fit%d%d",b1,b2));
  }

}

void eclipsederive()
{
  bool data = false;

  macro m("eclipsederiveMC");

  derivefromNS(data);
  // derivefromNS(true);

  //derivefromprofile();

  auto ggg = new TGraph(meanb.size(),&meanb[0],&prob[0]);
  auto cc = getc();
  ggg->Draw("AP");
  SavePlots(cc,"probvsbin");



  vector<float> x = binmean;
  vector<float> xerr;
  vector<float> y,yerr;
  vector<float> z,zerr;


  for (unsigned i=1;i<binbounds.size();i++) {
    // int b1 = binbounds[i-1];
    // int b2 = binbounds[i];
    // x.push_back((b1+b2)/2);

    xerr.push_back(0);
    y.push_back(fs[i-1]->GetParameter(0));
    yerr.push_back(fs[i-1]->GetParError(0));

    z.push_back(fs[i-1]->GetParameter(1));
    zerr.push_back(fs[i-1]->GetParError(1));
  }
  auto c2 = getc();
  auto g= new TGraphErrors(x.size(),&x[0],&y[0],&xerr[0],&yerr[0]);
  g->Draw();
  SavePlots(c2,"par0cent");

  auto c3 = getc();
  auto g2 = new TGraphErrors(x.size(),&x[0],&z[0],&xerr[0],&zerr[0]);
  g2->Draw();
  SavePlots(c3,"par1cent");


  cout<<"bins = {";
  for (unsigned i=0;i<binmean.size();i++) {
    cout<<binmean[i];
    if (i<binmean.size()-1) cout<<",";
  }
  cout<<"};"<<endl;

  cout<<"vector<float> par0";
  if (data) cout<<"dt"; else cout<<"mc";
  cout<<" = {";
  for (unsigned i=1;i<binbounds.size();i++) {
    int b1 = binbounds[i-1];
    int b2 = binbounds[i];
    cout<<fs[i-1]->GetParameter(0);
    if (i<binbounds.size()-1) cout<<",";
  }
  cout<<"};"<<endl;

  cout<<"vector<float> par1";
  if (data) cout<<"dt"; else cout<<"mc";
  cout<<" = {";
  for (unsigned i=1;i<binbounds.size();i++) {
    int b1 = binbounds[i-1];
    int b2 = binbounds[i];
    cout<<fs[i-1]->GetParameter(1);
    if (i<binbounds.size()-1) cout<<",";
  }
  cout<<"};"<<endl;

}