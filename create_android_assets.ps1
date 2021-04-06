$compress = @{
LiteralPath= "init.lua", "data", "modules", "layouts"
CompressionLevel = "Fastest"
DestinationPath = "android\otclientv8\assets\data.zip"
}
Compress-Archive @compress -Force