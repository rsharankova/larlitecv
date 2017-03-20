# LARCV AND LARLITE DIRS
MY_LARLITE=/a/data/amsterdam/tmw23/sw/tagger/larlite
MY_LARCV=/a/data/amsterdam/tmw23/sw/tagger/LArCV
MY_LARLITECV=/a/data/amsterdam/tmw23/sw/tagger/larlitecv
export ANN_LIBDIR=/a/data/amsterdam/tmw23/sw/tagger/LArCV/app/ann_1.1.2/lib
export ANN_INCDIR=/a/data/amsterdam/tmw23/sw/tagger/LArCV/app/ann_1.1.2/include

#SETUP LARLITE
source ${MY_LARLITE}/config/setup.sh

# SETUP LARCV
source $MY_LARCV/configure.sh

# setup LARLITECV
source ${MY_LARLITECV}/configure.sh

export LD_LIBRARY_PATH=${MY_LARCV}/app/ann_1.1.2/lib:${LD_LIBRARY_PATH}
export PATH=${MY_LARLITECV}/app/TaggerCROI/bin:${PATH}