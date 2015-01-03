#!/usr/bin/ruby

$files = IO::readlines("./files").
   each { |line| line.chomp! }.
   reject {|line| line =~ /^\s*$/}

nl = "\n"
for fname in $files
   File.open("./html/#{fname.downcase}.html", "w") {
      |output|

      output << <<EOL
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN"
   "http://www.w3.org/TR/html4/strict.dtd">
  <html>
    <head>
      <link rel="stylesheet" type="text/css" href="gcboard.css" />
    <title>Generic Chess Board homepage</title>
  </head>

  <body>
<div id="page">
<div id="header"><img src="logo.jpg" width="646" height="106"
		      alt="Generic Chess Board" /></div>
<div id="menu">
  <ul>
EOL
      for linkname in $files
         output.puts ( linkname == fname ?
                       "<li><span>#{fname}</span></li>" :
                       "<li><a href=\"./#{linkname.downcase}.html\">#{linkname}</a></li>" )
      end
    
      output << <<EOL
  </ul>
<a href="http://sourceforge.net"><img src="http://sflogo.sourceforge.net/sflogo.php?group_id=100728&amp;type=2" width="125" height="37" border="0" alt="SourceForge.net Logo" style="margin-top: 15px"/></a>
</div>
<div id="contents">
EOL
      doc = `/usr/bin/pandoc ./#{fname.downcase}.txt`
      output.puts(doc)
      output << <<EOL
</div>
<div class="footer"></div>
</div>
</body></html>
EOL
   }
end