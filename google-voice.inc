<?php

class google_voice {

  protected $browser;

  protected $code;

  public function __construct() {
    require_once '/home/boombatower/software/drupal-7/sites/all/modules/browser/browser.inc';
    $this->browser = new Browser();
  }

  public function login($user, $pass) {
    $this->browser->post('https://www.google.com/voice', array('Email' => $user, 'Passwd' => $pass), 'Sign in');

    // Extract code.
    if ($code = $this->browser->getPage()->xpath('//input[@name="_rnr_se"]')) {
      $code = array_pop($code);
      $this->code = (string) $code['value'];
    }
    return !empty($this->code);
  }

  public function sms_messages() {
    // Load recent SMS XML.
    $this->browser->get('https://www.google.com/voice/inbox/recent/sms/');
    $root = @new SimpleXMLElement($this->browser->getContent());

    // Grab the JSON summary information.
    $summary = json_decode((string) $root->json);

    // Grab the HTML part and import to SimpleXML.
    $dom = @DOMDocument::loadHTML((string) $root->html);
    $root = @simplexml_import_dom($dom);

    // Extract all messages.
    $parts = array('from', 'message', 'time');
    $converstations = array();
    foreach ($summary->messages as $message_id => $message_info) {
      $name = array_pop($root->xpath('//div[@id="' . $message_id . '"]//span[@class="gc-message-name"]'));
      $subject = preg_replace('/\s+/', ' ', trim(strip_tags($name->asXML())));

      $messages = array();
      if ($rows = $root->xpath('//div[@id="' . $message_id . '"]//div[@class="gc-message-sms-row"]')) {
        foreach ($rows as $row) {
          $message = array();
          for ($i = 0; $i < count($row->span); $i++) {
            $message[$parts[$i]] = trim((string) $row->span[$i]);
          }
          $message['from'] = rtrim($message['from'], ':');
          $messages[] = $message;
        }
      }

      $converstations[] = array(
        'subject' => $subject,
        'messages' => $messages,
      );
    }
    return $converstations;
  }

  public function sms_send($number, $message) {
    $text = array(
      'id' => '',
      'phoneNumber' => $number,
      'text' => $message,
      '_rnr_se' => $this->code,
    );
    $this->browser->curlExecute(array(
      CURLOPT_POST => TRUE,
      CURLOPT_HTTPHEADER => array('Expect' => ''),
      CURLOPT_URL => 'https://www.google.com/voice/sms/send/',
      CURLOPT_POSTFIELDS => http_build_query($text, NULL, '&'),
    ));

    return ($response = json_decode($this->browser->getContent())) && $response->ok;
  }
}
