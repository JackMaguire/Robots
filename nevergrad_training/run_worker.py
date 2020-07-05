from mpi4py import MPI
#from score_dofs import *
from robots import *

def run_worker( comm, rank, out_prefix ):

    while True:
        status = MPI.Status()
        #print( "waiting:", rank )
        dofs = comm.recv( source=0, tag=MPI.ANY_TAG, status=status )
        #print( "not waiting:", rank )
        if status.Get_tag() == 0:
            print( "Dying:", rank )
            comm.send( 0, dest=0, tag=0 )
            break

        final_score = score_similation( dofs )

        bundle = [ dofs, final_score ]
        comm.send( bundle, dest=0, tag=1 )
