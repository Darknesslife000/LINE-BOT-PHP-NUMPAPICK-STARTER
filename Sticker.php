 <?php
  

function Sticker_LINE($msg1 ,$msg2){
 $access_token = 'ONVddqupMYPMxbOdCLuqs/KZGks91b6Hnp7szQIDmAvIPmCsN4Lr/rqSiMTtNfMGeFEqVhDPJuSkYeG/1OI3hFLO6gD//vlgSkXPTNowltqK2Bvb6S6z7kesOVNGX9fgEtSfP8otd/S1Yya8RNlY5QdB04t89/1O/w1cDnyilFU='; 

  $messages = [
        type: "sticker",
        packageId: $msg1,
        stickerId: $msg2
      ];

      // Make a POST Request to Messaging API to reply to sender
      $url = 'https://api.line.me/v2/bot/message/push';
      $data = [

        'to' => 'U14ed3772ba0d4ddf60f84aa9b5bb9dba',
        'messages' => [$messages],
      ];
      $post = json_encode($data);
      $headers = array('Content-Type: application/json', 'Authorization: Bearer ' . $access_token);

      $ch = curl_init($url);
      curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "POST");
      curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
      curl_setopt($ch, CURLOPT_POSTFIELDS, $post);
      curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
      curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);
      $result = curl_exec($ch);
      curl_close($ch);

      echo $result . "\r\n"; 
}

?>
