#!/usr/bin/ruby
require 'net/http'

BASEDIR="./"

$files = IO::readlines(BASEDIR + "files").
  each { |line| line.chomp! }.
  reject {|line| line =~ "^\s*$"}


def write_page(title, filename, body)
  nl = "\n"
  File.open(filename, "w") { |output|

    output << <<EOL
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
  <html>
    <head>
    <title>Generic Chess Board Homepage</title>
  </head>

  <body>
    <table border=0 cellpadding=10 cellspacing=4 width="100%">
      <tr><td colspan=2 align="center">
             <img src="./gcb_logo.png" alt="Generic Chess Board Interface">
           </td></tr>
	  <tr><td bgcolor="#dddddd" valign="top" width=140>
EOL
    for linkname in $files
      output.puts "<a href=\"./#{linkname.downcase}.html\">#{linkname}</a><br>"
    end
    output.puts "<a href=\"./project.html\">Project Summary</a><br>"
    output.puts "<a href=\"./news.html\">News</a><br>"

    output << "</td><td>" << nl
    output << "<center><h2>" << title << "</h2></center>" << nl

    output << body
    
    output << "</td></tr></table>" << nl
    output << <<EOL
<a href="http://sourceforge.net"><img
   src="http://sourceforge.net/sflogo.php?group_id=100728&amp;type=4"
   width="125" height="37" border="0" alt="SourceForge.net Logo"
/></a></i><br>
EOL
    output << "<small>Last modified: " << Time.now << "</small>" << nl
    output << "</body></html>" << nl
  }
end

begin
  info = Net::HTTP.new("sourceforge.net", 80)
  body = info.get( "/export/projhtml.php?group_id=100728&mode=full&no_table=1", nil)[1]
  write_page("Project Summary", BASEDIR + "htdocs/project.html", body)
rescue
end

begin
  info = Net::HTTP.new("sourceforge.net", 80)
  body = info.get("/export/projnews.php?group_id=100728&limit=10&flat=1&show_summaries=1", nil)[1] 
  write_page("News", BASEDIR + "htdocs/news.html", body)
rescue
end
