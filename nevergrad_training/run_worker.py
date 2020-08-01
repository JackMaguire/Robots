from mpi4py import MPI
#from score_dofs import *
from robots import *

def run_worker( comm, rank, out_prefix ):

    i = 0
    best_score = -1.0
    
    while True:
        i += 1
        status = MPI.Status()
        #print( "waiting:", rank )
        dofs = comm.recv( source=0, tag=MPI.ANY_TAG, status=status )
        #print( "not waiting:", rank )
        if status.Get_tag() == 0:
            print( "Dying:", rank )
            comm.send( 0, dest=0, tag=0 )
            break

        potential_filename = str( comm ) + "_" + str(i)
        final_score = score_similation( dofs, potential_filename, best_score )
        best_score = min( best_score, final_score )
        
        bundle = [ dofs, final_score, potential_filename ]
        comm.send( bundle, dest=0, tag=1 )
