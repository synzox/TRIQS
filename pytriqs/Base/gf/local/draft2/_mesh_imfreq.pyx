
# ----------- Mesh  --------------------------
cdef class MeshImFreq: 
    cdef mesh_imfreq  _c

    def __init__(self, Beta, stat, int Nmax): 
        self._c =  make_mesh_imfreq(Beta,{'F' :Fermion, 'B' : Boson}[stat] ,Nmax) 
    
    def __len__ (self) : return self._c.size()
    
    property beta : 
        """Inverse temperature"""
        def __get__(self): return self._c.domain().beta
    
    property statistic : 
        def __get__(self): return 'F' if self._c.domain().statistic==Fermion else 'B'
    
    def __iter__(self) : # I use the C++ generator !
        cdef mesh_pt_generator[mesh_imfreq ] g = mesh_pt_generator[mesh_imfreq ](&self._c)
        while not g.at_end() : 
            yield g.to_point()
            g.increment()

    def __richcmp__(MeshImFreq self, MeshImFreq other,int op) : 
        if op ==2 : # ==
            return self._c == other._c

# C -> Python 
cdef inline make_MeshImFreq ( mesh_imfreq x) :
    return MeshImFreq(C_Object = encapsulate (&x))

