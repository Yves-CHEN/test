## Overview {: .expand}

### About
DENTIST (Detecting Errors iN summary staTISTics) was designed to evaluate and quality control summary statistics from association studies. Leveraging the LD information from an independent reference sample, DENTIST can distinguish the SNPs with inaccurate statistics in summary data, which are caused by poor genotyping/imputation as well as other artifacts occurred during an association study. As is assessed in our paper, a few of the frequently-used summary-based analysis, such as the meta-analysis, GCTA-COJO, SMR and LD score regression \[[PubMed ID: 25642630](https://www.ncbi.nlm.nih.gov/pubmed/25642630)\], can be biased to different degrees and improved with DENTIST-based QC.

**Latest release [v0.6.0](#Download) (22 Mar 2019)**

### Best practices

* Computing resources. DENTIST is better to run on multiple CPUs (ideal 6-12 CPUs) and 10Gb memory or larger depending on sample size of the LD reference. It is also a good idea to split the computation to each chromosome. Under these settings, when sample sizes are at several thousands and analyses are restricted to HAPMAP3 markers, DENTIST takes up to 2 hours for each chromosome.
* Summary statistics. Summary statistics can be either GWAS or eQTL summary data in the GCTA-COJO format. The per-marker association test is expected to be calculated from similar sample sizes, considering individuals can have missing genotypes at different markers. The allelic frequency check is performed by default using the frequencies provided from a summary data, although this allelic frequency consistency checking can be switch off.
* LD reference sample. The reference sample is expected to have matched ancestry to the sample used to produce the summary data. The sample size is expected to be > 1000.


### Credits 

The method was designed by [Wenhan Chen](mailto:uqwche11@uq.edu.au), [Zhihong Zhu](mailto:z.zhu1@uq.edu.au) and [Jian Yang](http://scholar.google.com.au/citations?user=aLuqQs8AAAAJ&hl=en). The software was implemented and has been maintained by Wenhan Chen.  The idea of the LD consistency test was originated from a previous study \[[PubMed ID: 24990607](https://www.ncbi.nlm.nih.gov/pubmed/24990607)\]. Webpage supports are from [Zhili Zheng](mailto:zhili.zheng@uq.edu.au).

### Questions and Help Requests 
If you have any bug reports or questions please send an email to  [Wenhan Chen](mailto:uqwche11@uq.edu.au) or [Jian Yang](mailto:jian.yang@uq.edu.au).

### Citations 
**DENTIST**  
In preparation. 

<p style="color: rgb(204,0,0);font-weight:bold;">Last update: 22 Mar 2019</p>




## Download
### Executable Files {: .notoc}

The executable file below is compiled with "-static", and tested on 64-bit Linux distributions on the x86\_64 CPU platform. 

Linux [DENTIST\_0.5.0.zip](./gcta_0.5.0.zip)

The executable file is released under the MIT license. 

> Note: The source code of DENTIST is still under frequent changes. It is now only obtainable by an email of request.




## Basic options

### Run
./DENTIST --bfile  test --gwas-summary ./gwas.sum.22.txt \
	--out tmp --thread-num 6


### Input and output

--bfile	test

Reads individual-level genotype data in PLINK bed format, e.g. test.fam, test.bim and test.bed.

--gwas-summary	summary.txt
Reads GWAS summary data in in GCTA-COJO format.
>Format of the GCTA-COJO file, summary.txt,
```nohighlight
SNP A1 A2 freq beta se p N
rs131538 A G 0.05 0.007 0.02  0.72  6000
rs140378 C G 0.05 0.005 0.016 0.754 6000
...  
```

--out tmp
Specifies the prefix of output file. In this case, it outputs "tmp.qc.DENTIST.txt" in the format of "rsID zGWAS zEst  R2  chisq  P  ifDup" as follows,
```nohighlight
rs131538        0.35	-0.01	0.432	0.51	0
rs140378	0.3	-0.9	14.4	1.47e-04	0
...  
```
For each marker specified by the rsID in the first column, zGWAS is the observed GWAS z-score. The zEst  is the z-score estimated based on the neighbor SNPs and Rsq is the multiple $R^{2} $ that measures how well the neighbor SNPs can tag this target SNP. The chisq is the chi-square test statistics calculated as (zGWAS - zEst)2/(1-R2). P is for p-value of the the chi-square test statistics. The ifDup marks a marker as 1 if it is in strong correlation (LD r2 >0.95) with any of its neighbor SNPs.

--maf 0.01
Specifies the MAF threshold, so that markers from the bfile with a MAF smaller than this value would be precluded from the analysis. This exclusion is performed after --target.


--target rs101
Is trailed by an rsID to specify a region of 20Mb of interest centered at position specified by rsID. 
The rsID should exist in the bfile. A warning is raised if the target rsID is not found. 
Notably, the identification of this rsID in the bfile is performed before  --maf flag.  




--wind  4000
Is trailed by the number of markers to specify the size of a sliding window. The default value is 4000 markers. This overrules --wind-dist option.

--wind-dist  2000000
Is trailed by sliding window size measured in the number of BP. The default value is 2000000 BP. This is the default option unless overruled by --wind.



--thread-num	4
Specifies the number of threads for parallel computing, given the tools is powered by OpenMP. The default value is 1.

--num-iterations 10
Specifies the number of iterations for LD consistency test (Method). The default value is 10. 

