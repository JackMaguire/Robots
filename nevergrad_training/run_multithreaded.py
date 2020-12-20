from concurrent import futures
import nevergrad as ng
import numpy as np
import time

from robots_mt import *

import argparse

parser = argparse.ArgumentParser(description='Running Nevergrad on multiple CPUs using MPI')
parser.add_argument('--opt', help='optimizer to use', required=True )
parser.add_argument('--budget', help='budget for optimizer', required=True, type=int )
#parser.add_argument('--hours', help='How long to run the simulation (budget just needs to be an estimate). This time does not include spin down time.', required=False, type=float, default=-1.0 )
parser.add_argument('--out_prefix', help='prefix for output files', required=True, type=str )
#parser.add_argument('--in_prefices', help='comma separated list of prefices to load from', required=False, type=str, default="" )
args = parser.parse_args()

all_results_dofs = []
all_results_scores = []

t0 = time.time()
best_score_seen = 0
best_dofs = 0

Params = parameterization_for_model( create_model() )
optimizer = ng.optimizers.registry[ args.opt ]( parametrization=Params, budget=args.budget, num_workers=95 )

class SpecialCallback:

    def __init__(self, out_prefix ) -> None:
        self.best_score = 0
        self.out_prefix = out_prefix

    def __call__(self, optimizer, candidate, value: float) -> None:
        if value < self.best_score:
            print( "new best score:", value )
            self.best_score = value
            dump_model( candidate, self.out_prefix + ".h5" )

callback = SpecialCallback( args.out_prefix )
optimizer.register_callback( "tell", callback )
            
with futures.ThreadPoolExecutor(max_workers=95) as executor:
    recommendation = optimizer.minimize(score_simulation, executor=executor, batch_mode=False)
print( "done" )


