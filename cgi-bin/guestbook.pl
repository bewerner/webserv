#!/usr/bin/perl
##############################################################################
# Guestbook                     Version 2.3.1                                #
# Copyright 1996 Matt Wright    mattw@scriptarchive.com                      #
# Created 4/21/95               Last Modified 10/29/95                       #
# Scripts Archive at:           http://www.scriptarchive.com/                #
##############################################################################
# COPYRIGHT NOTICE                                                           #
# Copyright 1996 Matthew M. Wright  All Rights Reserved.                     #
#                                                                            #
# Guestbook may be used and modified free of charge by anyone so long as     #
# this copyright notice and the comments above remain intact.  By using this #
# code you agree to indemnify Matthew M. Wright from any liability that      #  
# might arise from it's use.                                                 #  
#                                                                            #
# Selling the code for this program without prior written consent is         #
# expressly forbidden.  In other words, please ask first before you try and  #
# make money off of my program.                                              #
#                                                                            #
# Obtain permission before redistributing this software over the Internet or #
# in any other medium.	In all cases copyright and header must remain intact.#
##############################################################################
# Set Variables

use Cwd 'cwd';

# my $upload_dir = "/Users/bwerner/Documents/projects/rank05/webserv/github_webserv/html/uploads";

my $document_root = $ENV{'DOCUMENT_ROOT'} // cwd();  # Use DOCUMENT_ROOT if set, otherwise use the current directory

$guestbookurl = "http://localhost:8080/guestbook/guestbook.html";
$guestbookreal = "$document_root/guestbook/guestbook.html";
$guestlog = "$document_root/guestbook/guestlog.html";
$cgiurl = "http://localhost:8080/cgi-bin/guestbook.pl";
$date_command = "/bin/date";
# Set Your Options:
$mail = 0;              # 1 = Yes; 0 = No
$uselog = 1;            # 1 = Yes; 0 = No
$linkmail = 1;          # 1 = Yes; 0 = No
$separator = 1;         # 1 = <hr>; 0 = <p>
$redirection = 0;       # 1 = Yes; 0 = No
$entry_order = 1;       # 1 = Newest entries added first;
                        # 0 = Newest Entries added last.
$remote_mail = 0;       # 1 = Yes; 0 = No
$allow_html = 1;        # 1 = Yes; 0 = No
$line_breaks = 0;	# 1 = Yes; 0 = No

# If you answered 1 to $mail or $remote_mail you will need to fill out 
# these variables below:
$mailprog = '/usr/sbin/sendmail';
$recipient = 'you@your.com';

# Done
##############################################################################

# Get the Date for Entry
$date = `$date_command +"%A, %B %d, %Y at %T (%Z)"`; chop($date);
$shortdate = `$date_command +"%D %T %Z"`; chop($shortdate);

# Get the input
read(STDIN, $buffer, $ENV{'CONTENT_LENGTH'});

# Split the name-value pairs
@pairs = split(/&/, $buffer);

foreach $pair (@pairs) {
   ($name, $value) = split(/=/, $pair);

   # Un-Webify plus signs and %-encoding
   $value =~ tr/+/ /;
   $value =~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C", hex($1))/eg;
   $value =~ s/<!--(.|\n)*-->//g;

   if ($allow_html != 1) {
      $value =~ s/<([^>]|\n)*>//g;
   }

   $FORM{$name} = $value;
}

# Print the Blank Response Subroutines
&no_comments unless $FORM{'comments'};
&no_name unless $FORM{'realname'};

# Begin the Editing of the Guestbook File
open (FILE,"$guestbookreal") || die "Can't Open $guestbookreal: $!\n";
@LINES=<FILE>;
close(FILE);
$SIZE=@LINES;

# Open Link File to Output
open (GUEST,">$guestbookreal") || die "Can't Open $guestbookreal: $!\n";

for ($i=0;$i<=$SIZE;$i++) {
   $_=$LINES[$i];
   if (/<!--begin-->/) { 

      if ($entry_order eq '1') {
         print GUEST "<!--begin-->\n";
      }
   
      if ($line_breaks == 1) {
         $FORM{'comments'} =~ s/\cM\n/<br>\n/g;
      }

      print GUEST "<b class=\"comment\">$FORM{'comments'}</b>\n";

      if ($FORM{'url'}) {
         print GUEST "<a href=\"$FORM{'url'}\">$FORM{'realname'}</a>";
      }
      else {
         print GUEST "$FORM{'realname'}";
      }

      if ( $FORM{'username'} ){
         if ($linkmail eq '1') {
            print GUEST " \&lt;<a href=\"mailto:$FORM{'username'}\">";
            print GUEST "$FORM{'username'}</a>\&gt;";
         }
         else {
            print GUEST " &lt;$FORM{'username'}&gt;";
         }
      }

      print GUEST "<br>\n";

      if ( $FORM{'city'} ){
         print GUEST "$FORM{'city'},";
      }
     
      if ( $FORM{'state'} ){
         print GUEST " $FORM{'state'}";
      }

      if ( $FORM{'country'} ){
         print GUEST " $FORM{'country'}";
      }

      if ($separator eq '1') {
         print GUEST "<br>$date<hr>\n\n";
      }
      else {
         print GUEST " - $date<p>\n\n";
      }

      if ($entry_order eq '0') {
         print GUEST "<!--begin-->\n";
      }

   }
   else {
      print GUEST $_;
   }
}

close (GUEST);

# Log The Entry

if ($uselog eq '1') {
   &log('entry');
}


#########
# Options

# Mail Option
if ($mail eq '1') {
   open (MAIL, "|$mailprog $recipient") || die "Can't open $mailprog!\n";

   print MAIL "Reply-to: $FORM{'username'} ($FORM{'realname'})\n";
   print MAIL "From: $FORM{'username'} ($FORM{'realname'})\n";
   print MAIL "Subject: Entry to Guestbook\n\n";
   print MAIL "You have a new entry in your guestbook:\n\n";
   print MAIL "------------------------------------------------------\n";
   print MAIL "$FORM{'comments'}\n";
   print MAIL "$FORM{'realname'}";

   if ( $FORM{'username'} ){
      print MAIL " <$FORM{'username'}>";
   }

   print MAIL "\n";

   if ( $FORM{'city'} ){
      print MAIL "$FORM{'city'},";
   }

   if ( $FORM{'state'} ){
      print MAIL " $FORM{'state'}";
   }

   if ( $FORM{'country'} ){
      print MAIL " $FORM{'country'}";
   }

   print MAIL " - $date\n";
   print MAIL "------------------------------------------------------\n";

   close (MAIL);
}

if ($remote_mail eq '1' && $FORM{'username'}) {
   open (MAIL, "|$mailprog -t") || die "Can't open $mailprog!\n";

   print MAIL "To: $FORM{'username'}\n";
   print MAIL "From: $recipient\n";
   print MAIL "Subject: Entry to Guestbook\n\n";
   print MAIL "Thank you for adding to my guestbook.\n\n";
   print MAIL "------------------------------------------------------\n";
   print MAIL "$FORM{'comments'}\n";
   print MAIL "$FORM{'realname'}";

   if ( $FORM{'username'} ){
      print MAIL " <$FORM{'username'}>";
   }

   print MAIL "\n";

   if ( $FORM{'city'} ){
      print MAIL "$FORM{'city'},";
   }

   if ( $FORM{'state'} ){
      print MAIL " $FORM{'state'}";
   }

   if ( $FORM{'country'} ){
     print MAIL " $FORM{'country'}";
   }

   print MAIL " - $date\n";
   print MAIL "------------------------------------------------------\n";

   close (MAIL);
}

# Print Out Initial Output Location Heading
if ($redirection eq '1') {
   print "Location: $guestbookurl\n\n";
}
else { 
   &no_redirection;
}

#######################
# Subroutines

sub no_comments {
   print "Content-type: text/html\n\n";
   print "<html><head><title>No Comments</title></head>\n";
   print "<body><h1>Your Comments appear to be blank</h1>\n";
   print "The comment section in the guestbook fillout form appears\n";
   print "to be blank and therefore the Guestbook Addition was not\n";
   print "added.  Please enter your comments below.<p>\n";
   print "<form method=POST action=\"$cgiurl\">\n";
   print "Your Name:<input type=text name=\"realname\" size=30 ";
   print "value=\"$FORM{'realname'}\"><br>\n";
   print "E-Mail: <input type=text name=\"username\""; 
   print "value=\"$FORM{'username'}\" size=40><br>\n";
   print "City: <input type=text name=\"city\" value=\"$FORM{'city'}\" ";
   print "size=15>, State: <input type=text name=\"state\" "; 
   print "value=\"$FORM{'state'}\" size=15> Country: <input type=text "; 
   print "name=\"country\" value=\"$FORM{'country'}\" size=15><p>\n";
   print "Comments:<br>\n";
   print "<textarea name=\"comments\" COLS=60 ROWS=4></textarea><p>\n";
   print "<input type=submit> * <input type=reset></form><hr>\n";
   print "Return to the <a href=\"$guestbookurl\">Guestbook</a>.";
   print "\n</body></html>\n";

   # Log The Error
   if ($uselog eq '1') {
      &log('no_comments');
   }

   exit;
}

sub no_name {
   print "Content-type: text/html\n\n";
   print "<html><head><title>No Name</title></head>\n";
   print "<body><h1>Your Name appears to be blank</h1>\n";
   print "The Name Section in the guestbook fillout form appears to\n";
   print "be blank and therefore your entry to the guestbook was not\n";
   print "added.  Please add your name in the blank below.<p>\n";
   print "<form method=POST action=\"$cgiurl\">\n";
   print "Your Name:<input type=text name=\"realname\" size=30><br>\n";
   print "E-Mail: <input type=text name=\"username\"";
   print " value=\"$FORM{'username'}\" size=40><br>\n";
   print "City: <input type=text name=\"city\" value=\"$FORM{'city'}\" ";
   print "size=15>, State: <input type=text name=\"state\" ";
   print "value=\"$FORM{'state'}\" size=2> Country: <input type=text ";
   print "value=USA name=\"country\" value=\"$FORM{'country'}\" ";
   print "size=15><p>\n";
   print "Comments have been retained.<p>\n";
   print "<input type=hidden name=\"comments\" ";
   print "value=\"$FORM{'comments'}\">\n";
   print "<input type=submit> * <input type=reset><hr>\n";
   print "Return to the <a href=\"$guestbookurl\">Guestbook</a>.";
   print "\n</body></html>\n";

   # Log The Error
   if ($uselog eq '1') {
      &log('no_name');
   }

   exit;
}

# Log the Entry or Error
sub log {
   $log_type = $_[0];
   open (LOG, ">>$guestlog");
   if ($log_type eq 'entry') {
      print LOG "$ENV{'REMOTE_HOST'} - [$shortdate]<br>\n";
   }
   elsif ($log_type eq 'no_name') {
      print LOG "$ENV{'REMOTE_HOST'} - [$shortdate] - ERR: No Name<br>\n";
   }
   elsif ($log_type eq 'no_comments') {
      print LOG "$ENV{'REMOTE_HOST'} - [$shortdate] - ERR: No ";
      print LOG "Comments<br>\n";
   }
}

# Redirection Option
sub no_redirection {

   # Print Beginning of HTML
   print "Content-Type: text/html\n\n";
   print "<html><head><title>Thank You</title></head>\n";
   print "<body><h1>Thank You For Signing The Guestbook</h1>\n";

   # Print Response
   print "Thank you for filling in the guestbook.  Your entry has\n";
   print "been added to the guestbook.<hr>\n";
   print "Here is what you submitted:<p>\n";
   print "<b>$FORM{'comments'}</b><br>\n";

   if ($FORM{'url'}) {
      print "<a href=\"$FORM{'url'}\">$FORM{'realname'}</a>";
   }
   else {
      print "$FORM{'realname'}";
   }

   if ( $FORM{'username'} ){
      if ($linkmail eq '1') {
         print " &lt;<a href=\"mailto:$FORM{'username'}\">";
         print "$FORM{'username'}</a>&gt;";
      }
      else {
         print " &lt;$FORM{'username'}&gt;";
      }
   }

   print "<br>\n";

   if ( $FORM{'city'} ){
      print "$FORM{'city'},";
   }

   if ( $FORM{'state'} ){
      print " $FORM{'state'}";
   }

   if ( $FORM{'country'} ){
      print " $FORM{'country'}";
   }

   print " - $date<p>\n";

   # Print End of HTML
   print "<hr>\n";
   print "<a href=\"$guestbookurl\">Back to the Guestbook</a>\n";         print "- You may need to reload it when you get there to see your\n";
   print "entry.\n";
   print "</body></html>\n";

   exit;
}
