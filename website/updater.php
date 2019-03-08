<?php
$data_dir = "files";
$binary_path = "/otclient.exe";
$files_url = "http://otclient.ovh/files";
$checksums_file = "checksums.txt";
$update_checksum_interval = 60;

$data = array("url" => $files_url, "files" => array(), "binary" => $binary_path);

function getDirFiles($dir, &$results = array()){
    $files = scandir($dir);

    foreach($files as $key => $value){
        $path = realpath($dir.DIRECTORY_SEPARATOR.$value);
        if(!is_dir($path)) {
            $results[] = $path;
        } else if($value != "." && $value != "..") {
            getDirFiles($path, $results);            
        }
    }
    return $results;
}

function getChecksums() {
	global $data_dir;
	global $update_checksum_interval;
	
	$ret = array();
	$data_dir_realpath = realpath($data_dir);
	$files = getDirFiles($data_dir);
	foreach($files as $file) {
		$relative_path = str_replace($data_dir_realpath, "", $file);
		$ret[$relative_path] = md5_file($file);
	}	
	return $ret;
}

$files = getChecksums();
foreach($files as $file => $checksum) {
	$data["files"][$file] = $checksum;
}	
echo json_encode($data);

?>