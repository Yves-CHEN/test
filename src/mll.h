/************************************************************************
 *  Author: Wenhan CHEN
 *  Date  : 27 Oct 2014
 *  Last_Modified : 11 Dec 2014 14:11:26
 *  Description: This is a test for applying same optimazation but on depth.
  *
 *    NLopt package is MIT implementation of a set of optimization algorithms.
 *    Can be found at:
 *      http://ab-initio.mit.edu/wiki/index.php/NLopt_Algorithms#BOBYQA
 * 
************************************************************************/

#include <R.h>
#include <Rmath.h>
#include "utils.h"
#include "optimizer.h"
using namespace std;

class boundCon : public Constraint
{
public:
    int*      depths;
    int       T;
    double    del;
    double*   g_rcov_range;
    bool      lowerFlag;


    inline boundCon(int* depths, int T, double del, double* g_rcov_range, bool lowerFlag)
        :depths(depths), T(T), del(del), g_rcov_range(g_rcov_range), lowerFlag(lowerFlag)
    {
    }

    inline double lower(double rcov)
    {
        double val = g_rcov_range[0] - rcov;
        printf("applyin lower val = %f \n", val);
        return g_rcov_range[0] - rcov;
    }
    inline double  upper(double rcov)
    {
        double val = rcov - g_rcov_range[1];
        printf("applyin upper val = %f \n", val);
        return rcov - g_rcov_range[1] ;
    }
    inline double applyConstraint(const double* initParas)
    {
        double  tc   = initParas[0];
        double  DOA  = initParas[1];
        double  del  = initParas[2];  /// this overwrites this->del
        double  rcov = get_rcov(depths, T, tc, DOA, del);

        if(lowerFlag)
            return lower(rcov);
        else
            return upper(rcov);
    }

};

class  TFunc : public ObjectiveFunc
{

public:
    /// vector<double> initParameters; // inherited from ObjectiveFunc
    int*      genotypes;
    int*      depths;
    double*   pi;
    double*   filter;
    int       T;
    int       k;
    bool      doFilter;
    double*   g_rcov_range;
    double    del;

    vector< vector<double> >   tpm  ;
    vector< vector<double> >   yll  ;
    vector< vector<double> >   alpha;
    vector< vector<double> >   beta ;


    inline TFunc (int* depths, int* genotypes, double* pi, double** tpm, double* filter,
            int* numOfObser, int* numOfStates, vector<double> initPara, double del)
    :depths(depths), genotypes(genotypes), pi(pi), T(*numOfObser), k(*numOfStates), filter(filter), ObjectiveFunc(initPara), del(del)
    {
        doFilter = false;
        this->tpm.resize   (k, vector<double>(k, 0));
        for(int i = 0; i < k; i++)
        {
            for(int j = 0; j < k; j++)
            {
                this->tpm[i][j] = tpm[i][j] ;
            }
        }

        yll.resize   (k, vector<double>(T, 0));
        alpha.resize (k, vector<double>(T, 0));
        beta.resize  (k, vector<double>(T, 0));
    };

      
    inline double objective(const double* paras)
    {
        double tc  = paras[0];
        double DOA = paras[1];
        double del = this->del;
        //double del = paras[2];
        //double del = this->del;
        emissionDist(depths, T, genotypes, k, tc, DOA, yll, del);

        bool faster = true;
        bool print_info = false;
        double log_lik = forward_backward(yll, tpm, pi, doFilter, filter, alpha, beta, faster,print_info);

        Rprintf("\t [info] tc = %f\t DOA = %f when ll = %f, del = %f \n",
                tc, DOA, log_lik, del);
        return  log_lik;
    };
    
    vector<double> getSolution()
    {
        return ObjectiveFunc::initParameters;
    };
};


class MaxLikelihood : public Optimizer
{

public :
    /// Followings are parameters inherited from super.
    //ObjectiveFunc*  targetF;
    double lb[3];
    double ub[3];

    inline MaxLikelihood(ObjectiveFunc* targetF, double lb[2], double ub[2])
        :Optimizer(targetF)
    {
        this->lb[0] = lb[0];
        this->lb[1] = lb[1];
        this->lb[2] = lb[2];
        this->ub[0] = ub[0];
        this->ub[1] = ub[1];
        this->ub[2] = ub[2];

        
    };

    inline void doOptimize()
    {
        double paraMiniChange[3] = {1e-8, 1e-4, 1e-8};      // optimization parameter minimal changes each step.
        double maxTimes  = 800;
        
        //opt = nlopt_create(NLOPT_GN_ISRES, 3);   // Choose algorithm
        //opt = nlopt_create(NLOPT_GN_ORIG_DIRECT, 2);   // Choose algorithm
       //opt = nlopt_create(NLOPT_GN_ORIG_DIRECT_L, 3);   // Choose algorithm
        //opt = nlopt_create(NLOPT_LN_BOBYQA, 2);   // 
        //opt = nlopt_create(NLOPT_LN_NELDERMEAD, 2);   // 
        opt = nlopt_create(NLOPT_LN_SBPLX, 2);
        //opt = nlopt_create(NLOPT_LN_COBYLA, 3); 


        nlopt_set_lower_bounds(opt, lb);            // set stop criteria
        nlopt_set_upper_bounds(opt, ub);
        nlopt_set_xtol_rel(opt, 1e-12);             // this would not apply to Globle search.
        nlopt_set_xtol_abs(opt, paraMiniChange);    // as above
        nlopt_set_maxeval(opt, maxTimes);           // evaluate optimazation for < 100;
        Optimizer::doOptimize();

    }
};

//  *cc is an array of {tc_lowbound, tc_upbound}
//  *rr is an array of {ratio_lowbound, ratio_upbound}
//  g_rcov defines a globle relative coverage of tumor and normal.
//      This parameter determines the degree of deletion:
//        
void getTC_Ratio(double* filter, int* depth, int* genotypes, double** tpm, double* pi,
        int* numOfObser, int* numOfStates,
        double* tc_range,  double* res_tc,
        double* DOA_range, double* res_DOA,
        double* del_range, double* res_del,
        double* g_rcov_range)
{
    vector<double> paras ;
    /// set the mean value as the initial
    double lb[2] ;             // lower bounds.
    double ub[2] ;              // upper bounds.
    lb[0] = tc_range[0];
    ub[0] = tc_range[1];
    lb[1] = DOA_range[0];
    ub[1] = DOA_range[1];
    double initTC  = (tc_range[0]  + tc_range[1])/2;
    double initDOA = (DOA_range[0] + DOA_range[1])/2;

    paras.push_back(initTC);
    paras.push_back(initDOA);
    TFunc ff (depth, genotypes, pi, tpm, filter, numOfObser, numOfStates, paras, *res_del);
    boundCon con1(depth, *numOfObser, *res_del, g_rcov_range, true );  // lower flag
    boundCon con2(depth, *numOfObser, *res_del, g_rcov_range, false ); // upper flag
    ff.constraints.push_back(&con1);
    ff.constraints.push_back(&con2);


    MaxLikelihood mm (&ff, lb, ub);
    mm.doOptimize();
    paras = ff.initParameters;
    *res_tc  = paras[0];
    *res_DOA = paras[1];
    Rprintf("\t @ [info] finish opt  tc = %f\t DOA = %f del = %f\n",
                *res_tc, *res_DOA, *res_del);
}
void getTC_Ratio_2(double* filter, int* depth, int* genotypes, double** tpm, double* pi,
        int* numOfObser, int* numOfStates,
        double* tc_range,  double* res_tc,
        double* DOA_range, double* res_DOA,
        double* del_range, double* res_del,
        double* g_rcov_range)
{
    vector<double> paras ;
    /// set the mean value as the initial
    double lb[3] ;             // lower bounds.
    double ub[3] ;              // upper bounds.
    lb[0] = tc_range[0];
    ub[0] = tc_range[1];
    lb[1] = DOA_range[0];
    ub[1] = DOA_range[1];
    lb[2] = 0.000001;
    ub[2] = 0.999999;

//    double initTC  = (tc_range[0]  + tc_range[1])/2;
//    double initDOA = (DOA_range[0] + DOA_range[1])/2;
//    double initDel  = 0.5;
    double initTC  = *res_tc;
    double initDOA = *res_DOA;
    double initDel  = *res_del;


    initDel = 0.0001;


    printf("%f:%f:%f", initTC, initDOA, initDel);

    paras.push_back(initTC);
    paras.push_back(initDOA);
    paras.push_back(initDel);

    TFunc ff (depth, genotypes, pi, tpm, filter, numOfObser, numOfStates, paras, initDel);
    boundCon con1(depth, *numOfObser, *res_del, g_rcov_range, true );  // lower flag
    boundCon con2(depth, *numOfObser, *res_del, g_rcov_range, false ); // upper flag
    //ff.constraints.push_back(&con1);
    //ff.constraints.push_back(&con2);


    MaxLikelihood mm (&ff, lb, ub);
    mm.doOptimize();
    paras = ff.initParameters;
    *res_tc  = paras[0];
    *res_DOA = paras[1];
    //*res_del = paras[2];
    Rprintf("\t @ [info] finish opt  tc = %f\t DOA = %f del = %f\n",
                *res_tc, *res_DOA, *res_del);
}
