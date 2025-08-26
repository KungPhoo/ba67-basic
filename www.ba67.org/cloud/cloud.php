<?php

// save without BOM! It would send headers.

// Test:
// curl -v -sS -X LIST https://www.ba67.org/cloud.php -H"X-Auth: b503a69f442c66ea9a788ea3f8f1170f"


// allow accessing this from anywhere else than ba67.org
header('Access-Control-Allow-Origin: *');

// Allow all relevant methods
header("Access-Control-Allow-Methods: GET, POST, LIST, DELETE, OPTIONS");

// Allow headers commonly used in requests
header("Access-Control-Allow-Headers: *");

// Handle preflight (OPTIONS)
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}



// var_dump( get_defined_vars() );

$lockSymbol = "🔒";// \u1f512

// Create a per-user directory
$auth_key = $_SERVER['HTTP_X_AUTH'] ?? '';

if($auth_key == '' || strlen($auth_key)<16 || strlen($auth_key) > 512){
    http_response_code(400);
    echo "?CLOUD BAD USERNAME ERROR";
    exit;
}

$dataDir = __DIR__ . "/$auth_key";
if (!file_exists($dataDir)) {
    mkdir($dataDir, 0775, true);
}

function hasPassword($filePath){
    return file_exists($filePath.".PWD");
}
function filePassword($filePath){
    if(!hasPassword($filePath)){return "";}
    return file_get_contents($filePath.".PWD");
}

// https://stackoverflow.com/questions/478121/how-to-get-directory-size-in-php
function directorySize($path){
    $bytestotal = 0;
    $path = realpath($path);
    if($path!==false && $path!='' && file_exists($path)){
        foreach(new RecursiveIteratorIterator(new RecursiveDirectoryIterator($path, FilesystemIterator::SKIP_DOTS)) as $object){
            $bytestotal += $object->getSize();
        }
    }
    return $bytestotal;
}

// Parse request
$method = $_SERVER['REQUEST_METHOD'];
$filename = $_GET['file'] ?? null;

if($filename){
    // trim the lock symbol, in case someone copied it. (BA67 should do this)
    $filename = rtrim($filename, $lockSymbol);

    if (!preg_match('/^[a-zA-Z0-9._-]+\.bas$/i', $filename)) {
        http_response_code(400);
        echo "?CLOUD INVALID FILENAME ERROR";
        exit;
    }
    $filename = strtoupper($filename);
}

// Handle requests
switch ($method) {
    case 'GET':
        if ($filename) {
            $filePath = "$dataDir/$filename";
            if (file_exists($filePath)) {
                header('Content-Type: application/octet-stream');
                readfile($filePath);
                exit;
            }
        }
        http_response_code(404);
        echo "?CLOUD FILE NOT FOUND ERROR";
        break;

    case 'LIST':
        $files = array_values(array_diff(scandir($dataDir), ['.', '..']));
        header('Content-Type: application/octet-stream');
        foreach($files as $fn){
            if(!str_ends_with($fn, ".BAS")){continue;}

            $disp = $fn;
            if(hasPassword("$dataDir/$fn")){
                $disp .= $lockSymbol;
            }
            echo filesize("$dataDir/$fn")." ".$disp."\n";
        }
        // echo json_encode($files);
        break;

    case 'POST':
        if (!$filename) {
            http_response_code(400);
            echo "?CLOUD FILENAME REQUIRED ERROR";
            exit;
        }

        $password = "";
        if (isset($_GET['password'])) {
            $password = $_GET['password'];
        }

        $filePw = filePassword("$dataDir/$filename");
        if(strlen($filePw) > 0 && $filePw != $password){
            http_response_code(400);
            echo "?CLOUD PASSWORD INCORRECT ERROR";
            exit;
        }

        // Check extension (optionally with password)
        if (!preg_match('/\.bas$/i', $filename)) {
            http_response_code(400);
            echo "?CLOUD ONLY .BAS FILES ARE ALLOWED ERROR";
            exit;
        }

        // Limit to 128 KB
        // $rawData = file_get_contents('php://input', false, null, 0, 131072); // Read up to 128 KB + 1 byte
        $rawData = file_get_contents($_FILES["filedata1"]["tmp_name"], false, null, 0, 131072); // Read up to 128 KB + 1 byte
        if ($rawData === false) {
            http_response_code(500);
            echo "?CLOUD FAILED TO READ INPUT DATA ERROR";
            exit;
        }

        if (strlen($rawData) > 131072) {
            http_response_code(413); // Payload Too Large
            echo "?CLOUD FILE SIZE LIMIT ERROR";
            exit;
        }

        if(directorySize("$dataDir/") > 100 * 131072){
            http_response_code(413); // Payload Too Large
            echo "?CLOUD STORAGE FULL ERROR";
            exit;
            
        }


        // Save the file
        $filePath = "$dataDir/$filename";
        file_put_contents($filePath, $rawData);

        if(strlen($password) > 0){
            file_put_contents($filePath.".PWD", $password);
        }

        echo "SAVED TO CLOUD";
        break;

    case 'DELETE':
        if (!$filename) {
            http_response_code(400);
            echo "?CLOUD FILENAME NOT GIVEN ERROR";
            exit;
        }
        $filePath = "$dataDir/$filename";
        if (file_exists($filePath)) {
            unlink($filePath);
            echo "DELETED CLOUD FILE";
        } else {
            http_response_code(404);
            echo "?CLOUD FILE NOT FOUND ERROR";
        }
        break;

    default:
        http_response_code(405);
        echo "?CLOUD METHOD NOT FOUND ERROR. METHOD ".$method;
}
