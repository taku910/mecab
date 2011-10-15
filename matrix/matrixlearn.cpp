#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <fstream>
#include <iostream>

#include "tool.h"
#include "mecab.h"
#include "param.h"
#include "feature_index.h"
#include "lbfgs.h"
#include "strutil.h"
#include "common.h"

#define WHAT_FALSE2(a) do { cerr << a; return false; } while(0)

namespace MeCab {

  bool openID(const char *file,
	      std::vector<std::string> &out)
  {
    std::ifstream ifs (file);
    if (!ifs) 
      WHAT_FALSE2("cannot open: " << file);

    char buf [8192];
    char *col[4];
    int n = 0;
    out.clear();
    while (ifs.getline(buf, sizeof(buf))) {
      if (2 != tokenize(buf, "\t ", col, 2))
	WHAT_FALSE2("format error: " << buf);
      if (atoi(col[0]) != n++)
	WHAT_FALSE2("format error " << buf);
      out.push_back(col[1]);
    }

    return true;
  }

  bool openMatrix(const char *file,
		  std::vector<std::vector<std::pair<float, int *> > > & matrix) 
  {
    std::ifstream ifs (file);
    if (!ifs) 
      WHAT_FALSE2("cannot open: " << file);

    char buf [8192];
    char *col[4];

    ifs.getline(buf, sizeof(buf));
    if (2 != tokenize(buf, "\t ", col, 2))
      WHAT_FALSE2("format error: " << buf);

    size_t lsize = atoi(col[0]);
    size_t rsize = atoi(col[1]);

    matrix.resize(lsize);
    for (size_t i = 0; i < lsize; ++i) 
      matrix[i].resize(rsize);

    double sum = 0.0;
    while (ifs.getline(buf, sizeof(buf))) {
      if (3 != tokenize(buf, " ", col, 3))
	WHAT_FALSE2("format error: " << buf);
      size_t l = atoi(col[0]);
      size_t r = atoi(col[1]);
      double c = atof(col[2]);
      if (l >= lsize || r >= rsize)
	WHAT_FALSE2("given index is out of range: " << l << " " << r);
      matrix[l][r].first = c;
      sum += c;
    }

    for (size_t i = 0; i < lsize; ++i) 
      for (size_t j = 0; j < rsize; ++j) 
	matrix[i][j].first /= sum;

    return true;
  }

  bool Tool::learnMatrix(Param &param) 
  {
    double C   = param.getProfileFloat ("cost");
    double eta = param.getProfileFloat ("eta");

    if (C <= 0)
      WHAT_FALSE2("cost parameter is out of range");

    if (eta <= 0)
      WHAT_FALSE2("eta is out of range");

    std::vector<std::vector<std::pair<float, int *> > > matrix;
    std::vector<std::string> lids;
    std::vector<std::string> rids;

    if (! openID("left-id.def",  lids))
      WHAT_FALSE2("open faild: " << "left-id.def");

    if (! openID("right-id.def",  rids))
      WHAT_FALSE2("open faild: " << "right-id.def");

    if (! openMatrix("matrix.def",  matrix))
      WHAT_FALSE2("open faild: " << "matrix.def");

    size_t lsize = matrix.size();

    if (lsize == 0) 
      WHAT_FALSE2("matrix lsize == 0");

    size_t rsize = matrix[0].size();

    if (rsize == 0) 
      WHAT_FALSE2("matrix rsize == 0");

    if (lids.size() != lsize || rids.size() != rsize) 
      WHAT_FALSE2("matrix size and left/rigth size are diffrent" 
		  << lids.size() << " " << lsize << " "
		  << rids.size() << " " << rsize);

    // make feature
    EncoderFeatureIndex fi;
    LearnerPath path;
    LearnerNode rnode;
    LearnerNode lnode;
    path.lnode = &lnode;
    path.rnode = &rnode;
     
    if (! fi.open(param)) 
      WHAT_FALSE2(fi.what());
      
    rnode.stat  = lnode.stat = MECAB_NOR_NODE;
    for (size_t i = 0; i < lsize; ++i) {
      std::cout << i << std::endl;
      for (size_t j = 0; j < rsize; ++j) {
	path.lnode->rfeature = (char *)lids[i].c_str();
	path.rnode->lfeature = (char *)rids[j].c_str();
	fi.buildBigramFeature(&path);
	matrix[i][j].second = path.fvector;
      }
    }

    fi.clearcache ();

    int converge = 0;
    double old_obj = 0.0;
    size_t psize = fi.size();
    LBFGS lbfgs;

    std::vector<double> alpha(psize);
    std::vector<double> expected(psize);
    std::vector<double> observed(psize);
    std::fill (observed.begin(), observed.end(), 0.0);
    std::fill (alpha.begin(), alpha.end(), 0.0);

    fi.set_alpha(&alpha[0]);
    lbfgs.init ((int)psize, 5);
  
    for (size_t i = 0; i < lsize; ++i) 
      for (size_t j = 0; j < rsize; ++j) 
	for (int *f = matrix[i][j].second; *f != -1; ++f)
	  observed[*f] += matrix[i][j].first;

    std::cout << std::endl;
    std::cout << "Number of features:  " << psize    << std::endl;
    std::cout << "eta:                 " << eta      << std::endl;
    std::cout << "C(sigma^2):          " << C        << std::endl << std::endl;

    for (size_t itr = 0; ;  ++itr) {

      std::fill (expected.begin(), expected.end(), 0.0);

      double obj = 0.0;

      // calc expected
      double sum = 0.0;
      for (size_t i = 0; i < lsize; ++i) {
	for (size_t j = 0; j < rsize; ++j) {
	  double prob = 0.0;;
	  for (int *f = matrix[i][j].second; *f != -1; ++f)
	    prob += alpha[*f];
	  sum += std::exp(prob);
	}
      }

      for (size_t i = 0; i < lsize; ++i) {
	for (size_t j = 0; j < rsize; ++j) {
	  double prob = 0.0;;
	  for (int *f = matrix[i][j].second; *f != -1; ++f)
	    prob += alpha[*f];
	  prob = std::exp(prob) / sum;
	  for (int *f = matrix[i][j].second; *f != -1; ++f)
	    expected[*f] += prob;
	  obj -= matrix[i][j].first * std::log(prob);
	}
      }

      // update
      for (size_t i = 0; i < psize; ++i) {
	obj += (alpha[i] * alpha[i]/(2.0 * C));
	expected[i] = expected[i] - observed[i] + alpha[i]/C;
      }

      double diff = (itr == 0 ? 1.0 : std::fabs(1.0 * (old_obj - obj) )/old_obj);
      std::cout << "iter="    << itr
		<< " target=" << obj
		<< " diff="   << diff << std::endl;
      old_obj = obj;

      if (diff < eta) converge++; else converge = 0;
      if (converge == 3)  break; // 3 is ad-hoc

      int ret = lbfgs.optimize (&alpha[0], &obj, &expected[0]);

      if (ret <= 0) WHAT_FALSE2(lbfgs.what());
    }

    // output conditional prob
    {
      fi.save("model");

      std::ofstream ofs ("matrix.def2");
      if (!ofs) 
	WHAT_FALSE2("permission defined: " << "matrix.def");

      ofs << lsize << ' ' << rsize << std::endl;
      for (size_t i = 0; i < lsize; ++i) {
	double sum = 0.0;
	std::vector<double> prob(rsize);
	std::fill(prob.begin(), prob.end(), 0.0);
	for (size_t j = 0; j < rsize; ++j) {
	  for (int *f = matrix[i][j].second; *f != -1; ++f)
	    prob[j] += alpha[*f];
	  sum += (prob[j] = std::exp(prob[j]));
	}

	for (size_t j = 0; j < rsize; ++j) 
	  ofs << i << ' ' << j << ' ' << std::log(prob[j]/sum) << std::endl;
      }
    }

    return true;
  }
}

