#!/usr/bin/perl -w
use CGI;
use strict;
use warnings;
use File::Path qw(make_path);
use Cwd 'cwd';

# my $upload_dir = "/Users/bwerner/Documents/projects/rank05/webserv/github_webserv/html/uploads";

my $document_root = $ENV{'DOCUMENT_ROOT'} // cwd();  # Use DOCUMENT_ROOT if set, otherwise use the current directory
my $upload_dir = "$document_root/uploads";

# sleep 3;



# #TMP DEBUG
# # Open a file for writing
# open(my $fh, '>', 'env_output.txt') or die "Could not open file: $!";
# # Loop through each environment variable and write to file
# while (my ($key, $value) = each %ENV) {
#     print $fh "$key=$value\n";
# }






# Create the upload directory if it doesn't exist
unless (-d $upload_dir) {
    make_path($upload_dir) or die "Failed to create directory: $!";
}

my $query = CGI->new;

# Ensure parameters exist before processing
my $filename = $query->param("photo") || "";
my $email_address = $query->param("email_address") || "";

if (!$filename) {
    print $query->header(), "<html><body><p>Error: No file uploaded.</p></body></html>";
    exit;
}

# Sanitize filename (remove path)
$filename =~ s/.*[\/\\](.*)/$1/;

# Open filehandle safely
my $upload_filehandle = $query->upload("photo");

if (!$upload_filehandle) {
    print $query->header(), "<html><body><p>Error: Unable to read uploaded file.</p></body></html>";
    exit;
}

# Save file
open(my $UPLOADFILE, '>', "$upload_dir/$filename") or die "Cannot open file: $!";
while (<$upload_filehandle>) {
    print $UPLOADFILE $_;
}
close $UPLOADFILE;

# Print response
print $query->header();
print <<'END_HTML';
<html>
<head>
    <title>Thanks!</title>
</head>
<body>
    <p>Thanks for uploading your photo!</p>
    <p>Your email address: 
END_HTML
print $email_address;
print <<'END_HTML';
    </p>
    <p>Your photo:</p>
    <img src="/uploads/
END_HTML
print $filename;
print <<'END_HTML';
" border="0">
</body>
</html>
END_HTML
