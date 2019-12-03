<?php
 require("pub.php");
 require("line.php");
 require("Sticker.php");

// Get POST body content
$content = file_get_contents('php://input');
// Parse JSON

$events = json_decode($content, true);
// Validate parsed JSON data
if (!is_null($events['ESP'])) {
	
	$nub = strpos($events['ESP'], ',');
	$msgindex1 = substr($events['ESP'],$nub+1,5);
	$msgindex2 = substr($events['ESP'],$nub+7,8);
	$msgcut = ",".$msgindex1.",".$msgindex2;
	$events['ESP'] = str_replace($msgcut," ",$events['ESP']);

	send_LINE($events['ESP']);
	Sticker_LINE($msgindex1, $msgindex2);
		
	echo "OK";
	}
if (!is_null($events['events'])) {
	echo "line bot";
	// Loop through each event
	foreach ($events['events'] as $event) {
		// Reply only when message sent is in 'text' format
		if ($event['type'] == 'message' && $event['message']['type'] == 'text') {
			// Get text sent
			$text = $event['message']['text'];
			// Get replyToken
			$replyToken = $event['replyToken'];

			// Build message to reply back

			$Topic = "LineAlert" ;
			getMqttfromlineMsg($Topic,$text);
			   
			
		}
	}
}
$Topic = "LineAlert" ;
$text = "Income";
getMqttfromlineMsg($Topic,$text);
echo "OK3";
?>
