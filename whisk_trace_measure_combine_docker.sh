# ~/code/whisk/whisk_trace_measure_combine_docker.sh 'home/wanglab/data/whisker_asym' 'sc014/sc014_0325' 'sc014_0325_001_TopCam0.mp4' 'sc014_0325_001'
~/code/whisk/whisk_trace_measure_combine_docker.sh 'home/wanglab/data/whisker_asym' 'sc014/sc014_0324/test' 'sc014_0324_001_30sWhisking.mp4' 'sc014_0324_001'

#!/bin/bash                      
export HDF5_USE_FILE_LOCKING=FALSE

# Data info
projectDir=$1                               # project directory, e.g., data/whisker_asym
sessionDir=$2                               # session name, e.g., sc010/sc010_0207 
fName=$3                                    # video file name
baseName=$4                                 # used to save chunks, e.g., sc010_0207_3200
baseName="${baseName:='chunk'}"

# User info (in case this is run on someone else' data)

# Set directory
dataDir="$userDir/$projectDir/$sessionDir"  # where the data is
dataDir="${dataDir:=$PWD}"

echo "dataDir: $dataDir"
echo "fName: $fName"
echo "baseName: $baseName"

# Cutting video in halves then measure
cd $dataDir && \
mkdir -p "$dataDir/WT" && \
docker run --rm \
     -v $dataDir:/data -v /home/wanglab/code/whisk:/scripts \
     wanglabneuro/whisk-ww:nb-0.0.1 \
     python /scripts/cut_trace_measure.py --input /data/$fName --base $baseName --nproc 40
wait

# And combine to export
docker run --rm \
     -v $dataDir:/data -v /home/wanglab/code/whisk:/scripts \
     wanglabneuro/whisk-ww:nb-0.0.1 \
    python /scripts/combine_left_right_whiskers.py /data/WT

# debugging
# source /etc/profile.d/modules.sh
# module load openmind/singularity/3.6.3 
# cd /scratch2/scratch/Wed/vincent/whisker_asym/sc014/sc014_0324/test
# baseName='sc014_0324_001'
# fName='sc014_0324_001_30sWhisking.mp4'
# dataDir=$PWD

