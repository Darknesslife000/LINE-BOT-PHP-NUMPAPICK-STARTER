 <?php
  

function send_LINE($msg1, $msg2, $msg3){
 $access_token = 'ONVddqupMYPMxbOdCLuqs/KZGks91b6Hnp7szQIDmAvIPmCsN4Lr/rqSiMTtNfMGeFEqVhDPJuSkYeG/1OI3hFLO6gD//vlgSkXPTNowltqK2Bvb6S6z7kesOVNGX9fgEtSfP8otd/S1Yya8RNlY5QdB04t89/1O/w1cDnyilFU='; 

  $messages1 = [
        'type' => 'text',
        'text' => $msg1
        //'text' => $text
      ];
  $messages2 = [
        'type' => 'sticker',
        'packageId' => $msg2,
        'stickerId' => $msg3
      ];

      // Make a POST Request to Messaging API to reply to sender
      $url = 'https://api.line.me/v2/bot/message/push';
      $data1 = [

        'to' => 'U14ed3772ba0d4ddf60f84aa9b5bb9dba',
        'messages' => [$messages1],
      ];
      $data2 = [

        'to' => 'U14ed3772ba0d4ddf60f84aa9b5bb9dba',
        'messages' => [$messages2],
      ];
      $post1 = json_encode($data1);
      $post2 = json_encode($data2);
      $headers = array('Content-Type: application/json', 'Authorization: Bearer ' . $access_token);

      $ch = curl_init($url);
      curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "POST");
      curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
      curl_setopt($ch, CURLOPT_POSTFIELDS, $post1);
      curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
      curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);
      $result = curl_exec($ch);
      curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "POST");
      curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
      curl_setopt($ch, CURLOPT_POSTFIELDS, $post2);
      curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
      curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);
      $result = curl_exec($ch);
      curl_close($ch);

      echo $result . "\r\n"; 
}

?>
