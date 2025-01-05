tar -czf emenu.tar.gz $(git ls-files) $(cd slowdb/ && git ls-files | awk '{print "slowdb/"$1}')
