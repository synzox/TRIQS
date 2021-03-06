
################################################################################
#
# TRIQS: a Toolbox for Research in Interacting Quantum Systems
#
# Copyright (C) 2011 by M. Ferrero, O. Parcollet
#
# TRIQS is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# TRIQS is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# TRIQS. If not, see <http://www.gnu.org/licenses/>.
#
################################################################################

__all__ = ['HDF_Archive','HDF_Archive_Inert']
import sys,numpy,string
from HDF_Archive_basic_layer_h5py import HDF_Archive_group_basic_layer

from pytriqs.Base.Archive.HDF_Archive_Schemes import hdf_scheme_access, register_class

# -------------------------------------------
#
#  Various wrappers for basic python types.
#
# --------------------------------------------
def _my_str(ll, digs = 10) :
    ns = str(ll)
    for ii in xrange(digs-len(ns)): ns = '0'+ns
    return ns

class PythonListWrap:
    def __init__(self,ob) : 
        self.ob = ob
    def __reduce_to_dict__(self) : 
        return dict( [ (_my_str(n),v) for (n,v) in  enumerate (self.ob)])
    @classmethod
    def __factory_from_dict__(cls,D) :
        return [x for (n,x) in sorted(D.items())]

class PythonTupleWrap:
    def __init__(self,ob) : 
        self.ob = ob
    def __reduce_to_dict__(self) : 
        return dict( [ (_my_str(n),v) for (n,v) in  enumerate (self.ob)])
    @classmethod
    def __factory_from_dict__(cls,D) :
        return tuple([x for (n,x) in sorted(D.items())])

class PythonDictWrap:
    def __init__(self,ob) : 
        self.ob = ob
    def __reduce_to_dict__(self) : 
        return dict( [ (str(n),v) for (n,v) in self.ob.items()])
    @classmethod
    def __factory_from_dict__(cls,D) :
        return dict([(n,x) for (n,x) in D.items()])

register_class (PythonListWrap)
register_class (PythonTupleWrap)
register_class (PythonDictWrap)

# -------------------------------------------
#
#  A view of a subgroup of the archive 
#
# --------------------------------------------

class HDF_Archive_group (HDF_Archive_group_basic_layer) : 
    """
    """
    _wrappedType = { type([]) : PythonListWrap, type(()) : PythonTupleWrap, type({}) : PythonDictWrap}
    _MaxLengthKey = 500

    def __init__(self, parent, subpath) :
        self.options = parent.options
        HDF_Archive_group_basic_layer.__init__(self, parent, subpath)
        self.options = parent.options
        self.KeyAsStringOnly = self.options['KeyAsStringOnly']
        self._reconstruct_python_objects = self.options['ReconstructPythonObject']
        self.is_top_level = False
 
    #-------------------------------------------------------------------------
    def _key_cipher(self,key) : 
        if key in self.ignored_keys : 
            raise KeyError, "key %s is reserved"%key
        if self.KeyAsStringOnly : # for bacward compatibility
            if type(key) not in [ type('') , type(u'a')] : 
                raise KeyError, "Key must be string only !"
            return key
        r = repr(key)
        if len (r)> self._MaxLengthKey : 
            raise KeyError, "The Key is too large !"
        # check that the key is ok (it can be reconstructed)
        try : 
            ok = eval(r) == key 
        except : 
            ok =False
        if not ok :  raise KeyError, "The Key *%s*can not be serialized properly by repr !"%key
        return r

    #-------------------------------------------------------------------------
    def _key_decipher(self,key) : 
        return key if self.KeyAsStringOnly else eval(key)

    #-------------------------------------------------------------------------
    def __contains__(self,key) : 
        key= self._key_cipher(key)
        return key in self.keys()
    
    #-------------------------------------------------------------------------
    def values(self) :
        """
        Generator returning the values in the group
        """
        def res() : 
            for name in self.keys() :
                yield self[name]
        return res()

   #-------------------------------------------------------------------------
    def items(self) :
        """
        Generator returning couples (key, values) in the group.
        """
        def res() : 
            for name in self.keys():
                yield name, self[name]
        return res()

    #-------------------------------------------------------------------------
    def __iter__(self) :
        """Returns the keys, like a dictionary"""
        def res() : 
            for name in self.keys() :
                yield name
        return res()

    #-------------------------------------------------------------------------
    def __len__(self) :
        """Returns the length of the keys list """
        return  len(self.keys())
          
    #-------------------------------------------------------------------------
    def update(self,object_with_dict_protocol):
        for k,v in object_with_dict_protocol.items() : self[k] = v 
   
    #-------------------------------------------------------------------------
    def __delitem__(self,key) :
        key= self._key_cipher(key)
        self._clean_key(key,True) 
        self._cached_keys = None
 
    #-------------------------------------------------------------------------
    def __setitem__(self,key,val) :
        key= self._key_cipher(key)# first look if key is a string or key
       
        if key in self.keys() : 
            if self.options['do_not_overwrite_entries'] : raise KeyError, "key %s already exists"%key 
            self._clean_key(key) # clean things 

        # Transform list, dict, etc... into a wrapped type that will allow HDF reduction
        if type(val) in self._wrappedType: val = self._wrappedType[type(val)](val)

        # old way : deprecated
        def write_attributes1(g) : 
           g.write_attr("PythonClass",  val.__class__.__name__)
           g.write_attr("PythonModule",  val.__module__)
        
        # new way : preferred...
        def write_attributes(g) : 
           """Use the _hdf5_data_scheme_ if it exists otherwise the class name"""
           ds = val._hdf5_data_scheme_ if hasattr(val,"_hdf5_data_scheme_") else val.__class__.__name__  
           try : 
             sch = hdf_scheme_access(ds)
           except :
             err = """
               You are trying to store an object of type "%s", with the TRIQS_HDF5_data_scheme "%s". 
               But that data_scheme is not registered, so you will not be able to reread the class.
               Didn't you forget to register your class in pytriqs.Base.Archive.HDF_Archive_Schemes ?
               """ %(val.__class__.__name__,ds)
             raise IOError,err 
           g.write_attr("TRIQS_HDF5_data_scheme", ds)
 
        if '__write_hdf5__' in dir(val) : # simplest protocol
            val.__write_hdf5__(out,path2)
            write_attributes()
            #out.write("%s/@PythonClass"%path2,  val.__class__.__name__)
            #out.write("%s/@PythonModule"%path2,  val.__module__)
        elif '__reduce_to_dict__' in dir(val) : # Is it a HDF_compliant object 
            self.create_group(key) # create a new group
            d = val.__reduce_to_dict__() if '__reduce_to_dict__' in dir(val) else dict( [(x,getattr(val,x)) for x in val.__HDF_reduction__])
            if type(d) != type({}) : raise ValueError, " __reduce_to_dict__ method does not return a dict. See the doc !"
            if (d=={}) : raise ValueError, "__reduce_to_dict__ returns an empty dict"
            SUB = HDF_Archive_group(self,key)
            for n,v in d.items() : SUB[n] = v
            write_attributes(SUB)
            #out.write("%s/@PythonClass"%path2,  val.__class__.__name__)
            #out.write("%s/@PythonModule"%path2,  val.__module__)
        elif type(val)== numpy.ndarray : # it is a numpy
            try :
               self._write_array( key, numpy.array(val,copy=1,order='C') ) 
            except RuntimeError:
               print "HDF_Archive is in trouble with the array %s"%val
               raise
        elif isinstance(val, HDF_Archive_group) : # will copy the group recursively
            # we could add this for any object that has .items() in fact...
            SUB = HDF_Archive_group(self, key) 
            for k,v in val.items() : SUB[k]=v 
        else : # anything else... expected to be a scalar
            try :
               self._write_scalar( key, val)
            except:
               raise #ValueError, "Value %s\n is not of a type suitable to storage in HDF file"%val
        self._flush()
        self._cached_keys = None

    #-------------------------------------------------------------------------
    def get_raw (self,key):
        """Similar to __getitem__ but it does NOT reconstruct the python object, 
        it presents it as a subgroup"""
        return self.__getitem1__(key,False)
    #-------------------------------------------------------------------------
    def __getitem__(self,key) :
        """Return the object key, possibly reconstructed as a python object if
        it has been properly tagged with the PythonClass and PythonModule
        attributes"""
        return self.__getitem1__(key,self._reconstruct_python_objects)

    #-------------------------------------------------------------------------
    def __getitem1__(self,key,reconstruct_python_object) :
       
        if key not in self : 
            key = self._key_cipher(key)
            if key not in self  : raise KeyError, "Key %s does not exists"%key
 
        if self.is_group(key) :
            SUB = HDF_Archive_group(self,key) # View of the subgroup
            if not reconstruct_python_object : return SUB
            try : 
              hdf_data_scheme = SUB.read_attr("TRIQS_HDF5_data_scheme") 
              try : 
                sch = hdf_scheme_access(hdf_data_scheme)
              except :
                print "Warning : The TRIQS_HDF5_data_scheme %s is not recognized. Returning as a group"%hdf_data_scheme
                return SUB
              r_class_name  = sch.classname
              r_module_name = sch.modulename
            except:  # for backward compatibility
              try:
                r_class_name  = SUB.read_attr("PythonClass") 
                r_module_name = SUB.read_attr("PythonModule")
              except :
                return SUB
            #print  "%s  : python object of type %s.%s "%(path2,r_module_name,r_class_name)
            if not (r_class_name and r_module_name) : return SUB
            try :
                #r_class = getattr(my_import(r_module_name),r_class_name)
                exec("from %s import %s as r_class" %(r_module_name,r_class_name)) in globals(), locals()
            except KeyError : 
                raise RuntimeError, "I can not find the class %s to reconstruct the object !"%r_class_name
            if "__read_hdf5__" in dir(r_class) : 
                res = r_class()
                res.__read_hdf5__(self)
            elif "__factory_from_hdf5__" in dir(r_class) : 
                res = r_class.__factory_from_hdf5__(self)
            elif "__factory_from_dict__" in dir(r_class) : 
                f = lambda K : SUB.__getitem1__(K,reconstruct_python_object) if SUB.is_group(K) else SUB._read(K)
                values = dict( (self._key_decipher(K),f(K)) for K in SUB )
                res = r_class.__factory_from_dict__(values) 
            else : 
                raise ValueError, "Impossible to reread the class %s for group %s and key %s"%(r_class_name,self, key)
            return res
        elif self.is_data(key) : 
            return self._read(key)
        else : 
            raise KeyError, "Key %s is of unknown type !!"%Key 

    #---------------------------------------------------------------------------
    def __str__(self) :
        def pr(name) : 
            if self.is_group(name) :
                try :
                    # This is a python class
                    r_class_name  = self.read_attr("PythonClass") 
                    r_module_name = self.read_attr("PythonModule")
                    return "%s  : python object of type %s.%s "%(name,r_module_name,r_class_name)
                except : 
                    # this is an ordinary group
                    return "%s  : subgroup"%name
            elif self.is_data(name) : # can be an array of a number
                return "%s : Data "%(name)
            else :
                raise ValueError, "oopps %s"%(name)

        s= "HDF_Archive%s with the following content:\n"%(" (partial view)" if self.is_top_level else '')
        s+=string.join([ '  '+ pr(n) for n in self.keys() ], '\n')
        return s

    #-------------------------------------------------------------------------
    def __repr__(self) :
        return self.__str__()

    #------------------------------------------------------------------------- 
    def apply_on_leaves (self,f) : 
        """ 
           For each named leaf (name,value) of the tree, it calls f(name,value)
           f should return : 
            - `None`                    : no action is taken
            - an `empty tuple` ()       : the leaf is removed from the tree
            - an hdf-compliant value    : the leaf is replaced by the value
        """
        def visit_tree(n,d):
          for k in d:# Loop over the subgroups in d
              if d.is_group(k) : visit_tree(k,d[k])
              else : 
                  r = f(k,d[k])
                  if r != None : d[k] = r
                  elif r == () : del d[k]
        visit_tree('/',self['/'])


# -------------------------------------------
#
#  The main class 
#
# --------------------------------------------

class HDF_Archive (HDF_Archive_group) : 
    """
    """
    _class_version = "HDF_Archive | 1.0"

    def __init__(self, UrlName, Open_Flag = 'a', KeyAsStringOnly = True,
            ReconstructPythonObject = True, Init = {}): 
        r"""
           Parameters
           -----------
           UrlName : string
             The url of the hdf5 file.
              
                  * If url is a simple string, it is interpreted as a local file name
                  
                  * If url is a remote url (e.g. `http://ipht.cea.fr/triqs/data/SingleSiteBethe.output.h5` )
                    then the h5 file is downloaded in temporary file and opened.
                    In that case, ``Open_Flag`` must be 'r', read-only mode.
                    The temporary file is deleted at exit.
           Open_Flag : Legal modes: r, w, a (default)
           KeyAsStringOnly : True (default)
           Init : any generator of tuple (key,val), e.g. a dict.items().
             It will fill the archive with these values.

           Attributes
           ----------
           LocalFileName : string
             the name of the file or of the local downloaded copy
           UrlName : string
             the name of the Url 

           Examples
           --------
           >>> # retrieve a remove archive (in read-only mode) : 
           >>> h = HDF_Archive( 'http://ipht.cea.fr/triqs/data/SingleSiteBethe.output.h5') 
           >>>
           >>> # full copy of an archive
           >>> HDF_Archive( f, 'w', Init = HDF_Archive(fmp,'r').items())  # full
           >>>
           >>> # partial copy of file of name fmp, with only the key 'G'
           >>> HDF_Archive( f, 'w', Init = [ (k,v) for (k,v) in HDF_Archive(fmp,'r') if k in ['G'] )
           >>>
           >>> # faster version : the object are only retrieved when needed (list comprehension vs iterator comprehension) 
           >>> HDF_Archive( f, 'w', Init = ( (k,v) for (k,v) in HDF_Archive(fmp,'r') if k in ['G'] ) )
           >>> 
           >>> # partial copy with processing on the fly with the P function 
           >>> HDF_Archive( f, 'w', Init = ( (k,P(v)) for (k,v) in HDF_Archive(fmp,'r') if k in ['G'] ) )
           >>>
           >>> # another variant with a filtered dict
           >>> HDF_Archive( f, 'w', Init = HDF_Archive(fmp,'r').items(lambda k :  k in ['G'] ))

        """
        import os,os.path 
        assert Open_Flag in ['r','w','a'], "Invalid mode"
        assert type(UrlName)==type(''), "UrlName must be a string"

        # If it is an url , retrieve if and check mode is read only
        import urllib
        LocalFileName, http_message = urllib.urlretrieve (UrlName) if Open_Flag == 'r' else (UrlName, None)
        if LocalFileName != UrlName : # this was not a local file, so it must be read only
            assert Open_Flag == 'r', "You retrieve a distant Url %s which is not local, so it must be read-only. Use 'r' option"%UrlName
        
        if Open_Flag == 'w' : 
            os.system("rm -f %s"%(os.path.abspath(LocalFileName))) # destroys the file, ignoring errors

        self._init_root( LocalFileName, Open_Flag)    
        self.options = {'KeyAsStringOnly' : KeyAsStringOnly,
                        'do_not_overwrite_entries' : False,
                        'ReconstructPythonObject': ReconstructPythonObject,
                        'UseAlpsNotationForComplex'  : True
                        } 
        HDF_Archive_group.__init__(self,self,"")
        self.is_top_level = True
        for k,v in Init : self[k]=v

#--------------------------------------------------------------------------------

class HDF_Archive_Inert:
    """
    A fake class for the node in MPI. It does nothing, but 
    permits to write simply : 
       a= MPI.bcast(H['a']) # run on all nodes
    -[] : __getitem__ returns self so that H['a']['b'] is ok...
    - setitem : does nothing.
    """
    def HDF_Archive_Inert(self):
        pass
    def __getitem__(self,x)   : return self
    def __setitem__(self,k,v) : pass

#--------------------------------------------------------------------------------


