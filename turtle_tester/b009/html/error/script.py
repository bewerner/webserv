import os

# Define the directory where the files are located
directory = "/Users/bwerner/Documents/projects/rank05/webserv/github_webserv/html/error"

# Mapping of filenames to replacement text
replacement_map = {
	"400.html": "Bad Request",
	"401.html": "Unauthorized",
	"402.html": "Payment Required",
	"403.html": "Forbidden",
	"404.html": "Not Found",
	"405.html": "Method Not Allowed",
	"406.html": "Not Acceptable",
	"407.html": "Proxy Authentication Required",
	"408.html": "Request Timeout",
	"409.html": "Conflict",
	"410.html": "Gone",
	"411.html": "Length Required",
	"412.html": "Precondition Failed",
	"413.html": "Content Too Large",
	"414.html": "URI Too Long",
	"415.html": "Unsupported Media Type",
	"416.html": "Range Not Satisfiable",
	"417.html": "Expectation Failed",
	"418.html": "I'm a teapot",
	"421.html": "Misdirected Request",
	"422.html": "Unprocessable Content (WebDAV)",
	"423.html": "Locked (WebDAV)",
	"424.html": "Failed Dependency (WebDAV)",
	"425.html": "Too Early Experimental",
	"426.html": "Upgrade Required",
	"428.html": "Precondition Required",
	"429.html": "Too Many Requests",
	"431.html": "Request Header Fields Too Large",
	"451.html": "Unavailable For Legal Reasons",
	"501.html": "Not Implemented",
	"502.html": "Bad Gateway",
	"503.html": "Service Unavailable",
	"504.html": "Gateway Timeout",
	"505.html": "HTTP Version Not Supported",
	"506.html": "Variant Also Negotiates",
	"507.html": "Insufficient Storage (WebDAV)",
	"508.html": "Loop Detected (WebDAV)",
	"510.html": "Not Extended",
	"511.html": "Network Authentication Required"
}

# Loop through each file and replace "Not Found" if the file exists in the map
for filename, new_text in replacement_map.items():
	file_path = os.path.join(directory, filename)
	
	if os.path.exists(file_path):
		with open(file_path, "r", encoding="utf-8") as file:
			content = file.read()

		# Replace "Not Found" with the new status text
		updated_content = content.replace("Not Found", new_text)

		# Write the changes back to the file
		with open(file_path, "w", encoding="utf-8") as file:
			file.write(updated_content)
		
		# print(f"Updated {filename}: 'Not Found' '{new_text}'")
	else:
		print(f"File not found: {filename}")
