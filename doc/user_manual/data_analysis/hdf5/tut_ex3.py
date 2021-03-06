from pytriqs.Base.GF_Local import GFBloc_ReFreq 
from pytriqs.Base.GF_Local.Descriptors import SemiCircular 
from pytriqs.Base.Archive import HDF_Archive
import numpy

R = HDF_Archive('myfile.h5', 'w') 
for D in range(1,10,2) :
    g = GFBloc_ReFreq(Indices = [0], Beta = 50, MeshArray = numpy.arange(-1.99,2.00,0.02) , Name = "D=%s"%D)
    g <<=  SemiCircular(HalfBandwidth = 0.1*D)
    R[g.Name]= g


