mkdir JTAGBoundaryScanner
mkdir JTAGBoundaryScanner/Windows$1
cp ../LICENSE ./JTAGBoundaryScanner/ || exit 1
cp ../README.md ./JTAGBoundaryScanner/ || exit 1
cp ../Release-notes.txt ./JTAGBoundaryScanner/ || exit 1
cp *.dll ./JTAGBoundaryScanner/Windows$1/ || exit 1
cp *.exe ./JTAGBoundaryScanner/Windows$1/ || exit 1
cp ../lib_jtag_core/src/config.script ./JTAGBoundaryScanner/Windows$1/config.script || exit 1
mkdir JTAGBoundaryScanner/Windows$1/bsdl_files

zip -r JTAGBoundaryScanner_win$1.zip JTAGBoundaryScanner/
