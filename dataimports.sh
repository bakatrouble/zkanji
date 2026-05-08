#!/bin/bash

wget http://ftp.edrdg.org/pub/Nihongo/JMdict.gz -O - | gzip -d > dataimports/JMdict
wget http://www.edrdg.org/kanjidic/kanjidic.gz -O - | gzip -d | iconv -f EUC-JP -t UTF-8 > dataimports/kanjidic
wget http://ftp.edrdg.org/pub/Nihongo/radkfile.gz -O - | gzip -d | iconv -f EUC-JP -t UTF-8 > dataimports/radkfile
