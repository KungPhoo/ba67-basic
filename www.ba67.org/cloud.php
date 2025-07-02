<?php

// var_dump( get_defined_vars() );

// Create a per-user directory
$auth_key = $_SERVER['HTTP_X_AUTH'] ?? '';

if($auth_key == '' || strlen($auth_key)<16 || strlen($auth_key) > 512){
    http_response_code(400);
    echo "?CLOUD BAD USERNAME ERROR";
    exit;
}

$dataDir = __DIR__ . "/cloud/$auth_key";
if (!file_exists($dataDir)) {
    mkdir($dataDir, 0775, true);
}

// Parse request
$method = $_SERVER['REQUEST_METHOD'];
$filename = $_GET['file'] ?? null;

if($filename){
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
        foreach($files as $f){
            echo filesize("$dataDir/$f")." ".$f."\n";
        }
        // echo json_encode($files);
        break;

    case 'POST':
        if (!$filename) {
            http_response_code(400);
            echo "?CLOUD FILENAME REQUIRED ERROR";
            exit;
        }

        // Check extension
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

        // Save the file
        $filePath = "$dataDir/$filename";
        file_put_contents($filePath, $rawData);
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
