#! /bin/bash

echo "Content-Type: text/html"
echo ""

cat <<EOF
<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>Webserv</title>
	<style>
		body {
			display: flex;
			flex-direction: column;
			justify-content: center;
			align-items: center;
			min-height: 100vh;
			margin: 0;
			text-align: center;
			font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
			background-color: #232323;
		}
		p {
			white-space: pre-line;
			color: white;
			text-align: left;
		}
		.container {
			display: flex;
			flex-direction: column;
			align-items: center;
			gap: 30px;
			background: #2f2f2f;
			padding: 40px 60px;
			border-radius: 16px;
			box-shadow: 0 4px 12px rgba(3, 24, 7, 0.294);
			margin: 8px;
			width: max-content;
			min-width: 375px;
			max-width: 70%;
			min-width: 700px;
		}
		.title {
			font-size: 48px;
			font-weight: bold;
			color: #e4e4e4;
		}
		.message {
			font-size: 24px;
			color: #e4e4e4;
		}
		.flipped-emoji {
			transform: scaleX(-1);
			display: inline-block;
		}
		.button {
			text-decoration: none;
			display: block;
			text-align: center;
			padding: 12px 28px;
			font-size: 16px;
			font-weight: 500;
			border: none;
			border-radius: 80px;
			background-image: linear-gradient(to right, #065e14, #267536, #1e8824, #1a6f20);
			background-size: 300% 150%;
			color: #e4e4e4;
			cursor: pointer;
			box-shadow: 0 4px 12px rgba(3, 24, 7, 0.294);
			transition: all .4s ease-in-out;
		}
		.button:hover {
			background-position: 99% 0;
			transition: all .4s ease-in-out;
		}
	</style>
</head>
<body>
	<div class="container">
		<div class="title"><span class="flipped-emoji">🐢</span> CGI VARIABLES 🐢</div>
		<p>
EOF

echo "All arguments: $0 $@"
echo ""
echo "$(env)"

cat <<EOF
		</p>
		<a class="button" href="/">Home</a>
	</div>
</body>
</html>
EOF
