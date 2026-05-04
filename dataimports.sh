#!/bin/bash

wget http://ftp.edrdg.org/pub/Nihongo/JMdict.gz -O - | gzip -d > dataimports/JMdict
wget http://www.edrdg.org/kanjidic/kanjidic.gz -O - | gzip -d > dataimports/kanjidic
wget http://ftp.edrdg.org/pub/Nihongo/radkfile.gz -O - | gzip -d > dataimports/radkfile
