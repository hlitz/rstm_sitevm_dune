/* =============================================================================
 *
 * cluster.c
 *
 * =============================================================================
 *
 * Description:
 *
 * Takes as input a file, containing 1 data point per per line, and performs a
 * fuzzy c-means clustering on the data. Fuzzy clustering is performed using
 * min to max clusters and the clustering that gets the best score according to
 * a compactness and separation criterion are returned.
 *
 *
 * Author:
 *
 * Brendan McCane
 * James Cook University of North Queensland. Australia.
 * email: mccane@cs.jcu.edu.au
 *
 *
 * Edited by:
 *
 * Jay Pisharath, Wei-keng Liao
 * Northwestern University
 *
 * Chi Cao Minh
 * Stanford University
 *
 * =============================================================================
 *
 * For the license of bayes/sort.h and bayes/sort.c, please see the header
 * of the files.
 *
 * ------------------------------------------------------------------------
 *
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
 *
 * ------------------------------------------------------------------------
 *
 * For the license of ssca2, please see ssca2/COPYRIGHT
 *
 * ------------------------------------------------------------------------
 *
 * For the license of lib/mt19937ar.c and lib/mt19937ar.h, please see the
 * header of the files.
 *
 * ------------------------------------------------------------------------
 *
 * For the license of lib/rbtree.h and lib/rbtree.c, please see
 * lib/LEGALNOTICE.rbtree and lib/LICENSE.rbtree
 *
 * ------------------------------------------------------------------------
 *
 * Unless otherwise noted, the following license applies to STAMP files:
 *
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 */


#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "cluster.h"
#include "normal.h"
#include "random.h"
#include "util.h"
#include "tm.h"
#include "stm/lib_sitevm.h"


/* =============================================================================
 * extractMoments
 * =============================================================================
 */
static float*
extractMoments (float *data, int num_elts, int num_moments)
{
    int i;
    int j;
    float* moments;

    moments = (float*)sitecalloc(num_moments, sizeof(float));
    
    assert(moments);
    for (i = 0; i < num_elts; i++) {
        moments[0] += data[i];
    }

    moments[0] = moments[0] / num_elts;
    for (j = 1; j < num_moments; j++) {
        moments[j] = 0;
        for (i = 0; i < num_elts; i++) {
            moments[j] += pow((data[i]-moments[0]), j+1);
        }
        moments[j] = moments[j] / num_elts;
    }
    return moments;
}


/* =============================================================================
 * zscoreTransform
 * =============================================================================
 */
static void
zscoreTransform (float** data, /* in & out: [numObjects][numAttributes] */
                 int     numObjects,
                 int     numAttributes)
{
    float* single_variable;
    float* moments;
    int i;
    int j;
    int t = pthread_self();
    single_variable = (float*)calloc(numObjects, sizeof(float));
    //printf("starting clustersssssd %i %i %p\n", numAttributes, numObjects, single_variable);
    assert(single_variable);
    for (i = 0; i < numAttributes; i++) {
        for (j = 0; j < numObjects; j++) {
	  //printf("i %i j %i %i datap: %p\n", i , j, t, data);
	  //printf("%p %i\n", &(data[j][i]), t);
	  single_variable[j] = data[j][i];
	  //printf("single %p %i\n", &single_variable[j], t);  
        }
	//printf("starting clusterssseees\n");

        moments = extractMoments(single_variable, numObjects, 2);
        moments[1] = (float)sqrt((double)moments[1]);
	//printf("starting clusterssss33\n");

        for (j = 0; j < numObjects; j++) {
            data[j][i] = (data[j][i]-moments[0])/moments[1];
        }
        SEQ_FREE(moments);
    }
    SEQ_FREE(single_variable);
}


/* =============================================================================
 * cluster_exec
 * =============================================================================
 */
int
cluster_exec (
    int      nthreads,             /* in: number of threads*/
    int      numObjects,           /* number of input objects */
    int      numAttributes,        /* size of attribute of each object */
    float**  attributes,           /* [numObjects][numAttributes] */
    int      use_zscore_transform,
    int      min_nclusters,        /* testing k range from min to max */
    int      max_nclusters,
    float    threshold,            /* in:   */
    int*     best_nclusters,       /* out: number between min and max */
    float*** cluster_centres,      /* out: [best_nclusters][numAttributes] */
    int*     cluster_assign        /* out: [numObjects] */
)
{
    int itime;
    int nclusters;
    int* membership = 0;
    float** tmp_cluster_centres;
    random_t* randomPtr;
    TM_THREAD_ENTER();
    printf("starting clustersdd\n");
    //TM_BEGIN();
    membership = (int*)SEQ_MALLOC(numObjects * sizeof(int));
    assert(membership);

    randomPtr = random_alloc();
    assert(randomPtr);

    if (use_zscore_transform) {
        zscoreTransform(attributes, numObjects, numAttributes);
    }
    printf("starting clusterssss\n");

    itime = 0;
    //TM_END();
    /*
     * From min_nclusters to max_nclusters, find best_nclusters
     */

    for (nclusters = min_nclusters; nclusters <= max_nclusters; nclusters++) {
 printf("cluster 2\n");
   
        random_seed(randomPtr, 7);
	
        tmp_cluster_centres = normal_exec(nthreads,
                                          attributes,
                                          numAttributes,
                                          numObjects,
                                          nclusters,
                                          threshold,
                                          membership,
                                          randomPtr);
	TM_BEGIN();
        {
    printf("cluster\n");
            if (*cluster_centres) {
                SEQ_FREE((*cluster_centres)[0]);
                SEQ_FREE(*cluster_centres);
            }

            *cluster_centres = tmp_cluster_centres;
            *best_nclusters = nclusters;
        }
	
        itime++;
	TM_END();
    } /* nclusters */
    printf("cluster\n");
    TM_BEGIN();
    SEQ_FREE(membership);
    random_free(randomPtr);
    TM_END();
    return 0;
}


/* =============================================================================
 *
 * End of cluster.c
 *
 * =============================================================================
 */
