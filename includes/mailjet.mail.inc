<?php
/**
 * @file
 * Mail processing.
 */

class MailjetSmtpMailSystem implements MailSystemInterface {

  protected $AllowHtml;
  /**
   * Concatenate and wrap the e-mail body for either
   * plain-text or HTML emails.
   *
   * @param array $message
   *   A message array, as described in hook_mail_alter().
   *
   * @return string
   *   The formatted $message.
   */
  public function format(array $message) {
    $this->AllowHtml = \Drupal::config('mailjet.settings')->get('mailjet_allowhtml');
    // Join the body array into one string.
    $message['body'] = implode("\n\n", $message['body']);
    if (! $this->AllowHtml) {
      // Convert any HTML to plain-text.
      $message['body'] = drupal_html_to_text($message['body']);
      // Wrap the mail body for sending.
      $message['body'] = drupal_wrap_mail($message['body']);
    }
    return $message;
  }

  /**
   * Send the e-mail message.
   *
   * @see drupal_mail()
   *
   * @param array $message
   *   A message array, as described in hook_mail_alter().
   *
   * @return bool
   *   TRUE if the mail was successfully accepted, otherwise FALSE.
   */
  public function mail(array $message) {
    $id       = $message['id'];
    $to       = $message['to'];
    $from     = $message['from'];
    $subject  = $message['subject'];
    $body     = $message['body'];
    $headers  = $message['headers'];

    if (isset($message['params']['subject'])) {
      $subject = $message['params']['subject'];
    }

    if (isset($message['params']['body'][0])) {
      $body = $message['params']['body'][0];
    }

    if(!class_exists('PHPMailer')) {
      // If the PHPMailer class is not yet auto-loaded, try to load the library
      // using Libraries API, if present.
      if (function_exists('libraries_load')) {
        $library = libraries_load('phpmailer');
        if (empty($library) || empty($library['loaded'])) {
          \Drupal::logger('mailjet')->error('Unable to send mail : Libraries API can not load PHPMailer library.', array());
          drupal_set_message(t('Unable to send mail : PHPMailer library does not exist.'), 'error');
          return FALSE;
        }
      }
      // Libraries API not present, try manually loading the class file.
      elseif (file_exists('sites/all/libraries/phpmailer/PHPMailerAutoload.php')) {
        require_once 'sites/all/libraries/phpmailer/PHPMailerAutoload.php';
      }
      else {
        drupal_set_message(t('Unable to send mail : PHPMailer library does not exist.'), 'error');
        \Drupal::logger('mailjet')->error('Unable to send mail : Libraries API and PHPMailer library does not exist .', array());
        return FALSE;
      }
    }

    // Create a new PHPMailer object - autoloaded from registry.
    $mailer = new PHPMailer();

    // @FIXME
// // @FIXME
// // This looks like another module's variable. You'll need to rewrite this call
// // to ensure that it uses the correct configuration object.
// $from_name = variable_get('site_name', '');


    // Hack to fix reply-to issue.
    // @FIXME
// // @FIXME
// // This looks like another module's variable. You'll need to rewrite this call
// // to ensure that it uses the correct configuration object.
// $properfrom = variable_get('site_mail', '');

    if (!empty($properfrom)) {
      $headers['From'] = $properfrom;
    }
    if (!isset($headers['Reply-To']) || empty($headers['Reply-To'])) {
      if (strpos($from, '<')) {
        $reply = preg_replace('/>.*/', '', preg_replace('/.*</', '', $from));
      }
      else {
        $reply = $from;
      }
      $headers['Reply-To'] = $reply;
    }

    // Blank value will let the e-mail address appear.
    if ($from == NULL || $from == '') {
      // If from e-mail address is blank, use smtp_from config option.
      // @FIXME
// // @FIXME
// // This looks like another module's variable. You'll need to rewrite this call
// // to ensure that it uses the correct configuration object.
// if (($from = variable_get('site_from', '')) == '') {
//         // If smtp_from config option is blank, use site_email.
//         if (($from = variable_get('site_mail', '')) == '') {
//           drupal_set_message(t('There is no submitted from address.'), 'error');
// 
//           if (variable_get('mailjet_debug')) {
//             watchdog('mailjet', 'There is no submitted from address.', array(), WATCHDOG_ERROR);
//           }
// 
//           return FALSE;
//         }
//       }

    }
    if (preg_match('/^"?.*"?\s*<.*>$/', $from)) {
      // . == Matches any single character except line break characters \r and
      // \n.
      // * == Repeats the previous item zero or more times.
      $from_name = preg_replace('/"?([^("\t\n)]*)"?.*$/', '$1', $from);
      $from      = preg_replace("/(.*)\<(.*)\>/i", '$2', $from);
    }
    elseif (!valid_email_address($from)) {
      drupal_set_message(t('The submitted from address (@from) is not valid.', array('@from' => $from)), 'error');

      // @FIXME
// Could not extract the default value because it is either indeterminate, or
// not scalar. You'll need to provide a default value in
// config/install/mailjet.settings.yml and config/schema/mailjet.schema.yml.
if (\Drupal::config('mailjet.settings')->get('mailjet_debug')) {
        \Drupal::logger('mailjet')->error('The submitted from address (@from) is not valid.', array('@from' => $from));
      }

      return FALSE;
    }

    // Defines the From value $from_nameto what we expect.
    $mailer->SetFrom($from, $from_name);

    // Create the list of 'To:' recipients.
    $torecipients = explode(',', $to);
    foreach ($torecipients as $torecipient) {
      if (strpos($torecipient, '<') !== FALSE) {
        $toparts = explode(' <', $torecipient);
        $toname = $toparts[0];
        $toaddr = rtrim($toparts[1], '>');
      }
      else {
        $toname = '';
        $toaddr = $torecipient;
      }
      $mailer->AddAddress($toaddr, $toname);
    }


    // Parse the headers of the message and set the PHPMailer object's settings
    // accordingly.
    foreach ($headers as $key => $value) {
      switch (\Drupal\Component\Utility\Unicode::strtolower($key)) {
        case 'from':
          if ($from == NULL or $from == '') {
            // If a from value was already given, then set based on header.
            // Should be the most common situation since drupal_mail moves the
            // from to headers.
            $from           = $value;
            $mailer->From     = $value;
            // Then from can be out of sync with from_name !
            $mailer->FromName = '';
            $mailer->Sender   = $value;
          }
          break;

        case 'content-type':
          // Parse several values on the Content-type header, storing them in
          // an array like key=value -> $vars['key']='value'
          $vars = explode('; ', $value);
          foreach ($vars as $i => $var) {
            if ($cut = strpos($var, '=')) {
              $new_var = \Drupal\Component\Utility\Unicode::strtolower(\Drupal\Component\Utility\Unicode::substr($var, $cut + 1));
              $new_key = \Drupal\Component\Utility\Unicode::substr($var, 0, $cut);
              unset($vars[$i]);
              $vars[$new_key] = $new_var;
            }
          }

          // Set the charset based on the provided value, if there is one.
          $mailer->CharSet = isset($vars['charset']) ? $vars['charset'] : 'utf-8';

          switch ($vars[0]) {
            case 'text/plain':
              // The message includes only a plain text part.
              $mailer->IsHTML(FALSE);
              $content_type = 'text/plain';
              break;

            case 'text/html':
              // The message includes only an HTML part.
              $mailer->IsHTML(TRUE);
              $content_type = 'text/html';
              break;

            case 'multipart/related':
              // Get the boundary ID from the Content-Type header.
              $boundary = $this->getSubstrings($value, 'boundary', '"', '"');

              // The message includes an HTML part w/inline attachments.
              $mailer->ContentType = $content_type = 'multipart/related; boundary="' . $boundary . '"';
              break;

            case 'multipart/alternative':
              // The message includes both a plain text and an HTML part.
              $mailer->ContentType = $content_type = 'multipart/alternative';

              // Get the boundary ID from the Content-Type header.
              $boundary = $this->getSubstrings($value, 'boundary', '"', '"');
              break;

            case 'multipart/mixed':
              // The message includes one or more attachments.
              $mailer->ContentType = $content_type = 'multipart/mixed';

              // Get the boundary ID from the Content-Type header.
              $boundary = $this->getSubstrings($value, 'boundary', '"', '"');
              break;

            default:
              // Everything else is unsuppored by PHPMailer.
              drupal_set_message(t('The Content-Type of your message is not supported by PHPMailer and will be sent as text/plain instead.'), 'error');

              // @FIXME
// Could not extract the default value because it is either indeterminate, or
// not scalar. You'll need to provide a default value in
// config/install/mailjet.settings.yml and config/schema/mailjet.schema.yml.
if (\Drupal::config('mailjet.settings')->get('mailjet_debug')) {
                  \Drupal::logger('mailjet')->error('The Content-Type of your message is not supported by PHPMailer and will be sent as text/plain instead.', array());
              }

              // Force the Content-Type to be text/plain.
              $mailer->IsHTML(FALSE);
              $content_type = 'text/plain';
          }
          break;

        case 'reply-to':
          // Only add a "reply-to" if it's not the same as "return-path".
          if ($value != $headers['Return-Path']) {
            if (strpos($value, '<') !== FALSE) {
              $reply_to_parts = explode('<', $value);
              $reply_to_name = trim($reply_to_parts[0]);
              $reply_to_name = trim($reply_to_name, '"');
              $reply_to_addr = rtrim($reply_to_parts[1], '>');
              $mailer->AddReplyTo($reply_to_addr, $reply_to_name);
            }
            else {
              $mailer->AddReplyTo($value);
            }
          }
          break;

        case 'content-transfer-encoding':
          $mailer->Encoding = $value;
          break;

        case 'return-path':
        case 'mime-version':
        case 'x-mailer':
          break;

        case 'errors-to':
          $mailer->AddCustomHeader('Errors-To: ' . $value);
          break;

        case 'cc':
          $ccrecipients = explode(',', $value);
          foreach ($ccrecipients as $ccrecipient) {
            if (strpos($ccrecipient, '<') !== FALSE) {
              $ccparts = explode(' <', $ccrecipient);
              $ccname = $ccparts[0];
              $ccaddr = rtrim($ccparts[1], '>');
            }
            else {
              $ccname = '';
              $ccaddr = $ccrecipient;
            }
            $mailer->AddBCC($ccaddr, $ccname);
          }
          break;

        case 'bcc':
          $bccrecipients = explode(',', $value);
          foreach ($bccrecipients as $bccrecipient) {
            if (strpos($bccrecipient, '<') !== FALSE) {
              $bccparts = explode(' <', $bccrecipient);
              $bccname = $bccparts[0];
              $bccaddr = rtrim($bccparts[1], '>');
            }
            else {
              $bccname = '';
              $bccaddr = $bccrecipient;
            }
            $mailer->AddBCC($bccaddr, $bccname);
          }
          break;

        default:
          // The header key is not special - add it as is.
          $mailer->AddCustomHeader($key . ': ' . $value);
      }
    }

    $mailer->AddCustomHeader('X-Mailer:Mailjet-for-Drupal/1.0');

    $mailer->Subject = $subject;

    // Processes the message's body.
    switch ($content_type) {
      case 'multipart/related':
        $mailer->Body = $body;

        break;

      case 'multipart/alternative':
        // Split the body based on the boundary ID.
        $body_parts = $this->boundarySplit($body, $boundary);
        foreach ($body_parts as $body_part) {
          // If plain/text within the body part, add it to $mailer->AltBody.
          if (strpos($body_part, 'text/plain')) {
            // Clean up the text.
            $body_part = trim($this->removeHeaders(trim($body_part)));
            // Include it as part of the mail object.
            $mailer->AltBody = $body_part;
          }
          // If plain/html within the body part, add it to $mailer->Body.
          elseif (strpos($body_part, 'text/html')) {
            // Clean up the text.
            $body_part = trim($this->removeHeaders(trim($body_part)));
            // Include it as part of the mail object.
            $mailer->Body = $body_part;
          }
        }
        break;

      case 'multipart/mixed':
        // Split the body based on the boundary ID.
        $body_parts = $this->boundarySplit($body, $boundary);

        // Determine if there is an HTML part for when adding the plain
        // text part.
        $text_plain = FALSE;
        $text_html  = FALSE;
        foreach ($body_parts as $body_part) {
          if (strpos($body_part, 'text/plain')) {
            $text_plain = TRUE;
          }
          if (strpos($body_part, 'text/html')) {
            $text_html = TRUE;
          }
        }

        foreach ($body_parts as $body_part) {
          // If test/plain within the body part, add it to either
          // $mailer->AltBody or $mailer->Body, depending on whether there is
          // also a text/html part ot not.
          if (strpos($body_part, 'multipart/alternative')) {
            // Clean up the text.
            $body_part = trim($this->removeHeaders(trim($body_part)));
            // Get boundary ID from the Content-Type header.
            $boundary2 = $this->getSubstrings($body_part, 'boundary', '"', '"');
            // Split the body based on the boundary ID.
            $body_parts2 = $this->boundarySplit($body_part, $boundary2);

            foreach ($body_parts2 as $body_part2) {
              // If plain/text within the body part, add it to $mailer->AltBody.
              if (strpos($body_part2, 'text/plain')) {
                // Clean up the text.
                $body_part2 = trim($this->removeHeaders(trim($body_part2)));
                // Include it as part of the mail object.
                $mailer->AltBody = $body_part2;
                $mailer->ContentType = 'multipart/mixed';
              }
              // If plain/html within the body part, add it to $mailer->Body.
              elseif (strpos($body_part2, 'text/html')) {
                // Clean up the text.
                $body_part2 = trim($this->removeHeaders(trim($body_part2)));
                // Include it as part of the mail object.
                $mailer->Body = $body_part2;
                $mailer->ContentType = 'multipart/mixed';
              }
            }
          }
          // If text/plain within the body part, add it to $mailer->Body.
          elseif (strpos($body_part, 'text/plain')) {
            // Clean up the text.
            $body_part = trim($this->removeHeaders(trim($body_part)));

            if ($text_html) {
              $mailer->AltBody = $body_part;
              $mailer->IsHTML(TRUE);
              $mailer->ContentType = 'multipart/mixed';
            }
            else {
              $mailer->Body = $body_part;
              $mailer->IsHTML(FALSE);
              $mailer->ContentType = 'multipart/mixed';
            }
          }
          // If text/html within the body part, add it to $mailer->Body.
          elseif (strpos($body_part, 'text/html')) {
            // Clean up the text.
            $body_part = trim($this->removeHeaders(trim($body_part)));
            // Include it as part of the mail object.
            $mailer->Body = $body_part;
            $mailer->IsHTML(TRUE);
            $mailer->ContentType = 'multipart/mixed';
          }
          // Add the attachment.
          elseif (strpos($body_part, 'Content-Disposition: attachment;')) {
            $file_path     = $this->getSubstrings($body_part, 'filename=', '"', '"');
            $file_name     = $this->getSubstrings($body_part, ' name=', '"', '"');
            $file_encoding = $this->getSubstrings($body_part, 'Content-Transfer-Encoding', ' ', "\n");
            $file_type     = $this->getSubstrings($body_part, 'Content-Type', ' ', ';');

            if (file_exists($file_path)) {
              if (!$mailer->AddAttachment($file_path, $file_name, $file_encoding, $filetype)) {
                drupal_set_message(t('Attahment could not be found or accessed.'));
              }
            }
            else {
              // Clean up the text.
              $body_part = trim($this->removeHeaders(trim($body_part)));

              if (\Drupal\Component\Utility\Unicode::strtolower($file_encoding) == 'base64') {
                $attachment = base64_decode($body_part);
              }
              elseif (\Drupal\Component\Utility\Unicode::strtolower($file_encoding) == 'quoted-printable') {
                $attachment = quoted_printable_decode($body_part);
              }
              else {
                $attachment = $body_part;
              }

              $attachment_new_filename = tempnam(realpath(file_directory_temp()), 'smtp');
              $file_path               = file_save_data($attachment, $attachment_new_filename, FILE_EXISTS_RENAME);

              if (!$mailer->AddAttachment($file_path, $file_name)) {
                drupal_set_message(t('Attachment could not be found or accessed.'));
              }
            }
          }
        }
        break;

      default:
        $mailer->Body = $body;
        break;
    }

    // Set the authentication settings.
    $username = \Drupal::config('mailjet.settings')->get('mailjet_username');
    $password = \Drupal::config('mailjet.settings')->get('mailjet_password');

    // If username and password are given, use SMTP authentication.
    if ($username != '' && $password != '') {
      $mailer->SMTPAuth = TRUE;
      $mailer->Username = $username;
      $mailer->Password = $password;
    }

    // Set the protocol prefix for the smtp host.
    switch (\Drupal::config('mailjet.settings')->get('mailjet_protocol')) {
      case 'ssl':
        $mailer->SMTPSecure = 'ssl';
        break;

      case 'tls':
        $mailer->SMTPSecure = 'tls';
        break;

      default:
        $mailer->SMTPSecure = '';
    }

    // Set other connection settings.
    //$mailer->Host   = variable_get('mailjet_host', 'pp-leela1.mailjet.com');
    $mailer->Host   = \Drupal::config('mailjet.settings')->get('mailjet_host');
    $mailer->Port   = \Drupal::config('mailjet.settings')->get('mailjet_port');
    $mailer->Mailer = 'smtp';

    // @FIXME
// Could not extract the default value because it is either indeterminate, or
// not scalar. You'll need to provide a default value in
// config/install/mailjet.settings.yml and config/schema/mailjet.schema.yml.
if (\Drupal::config('mailjet.settings')->get('mailjet_debug')) {
        \Drupal::logger('mailjet')->notice('Sending mail to: @to', array('@to' => $to));
    }

    // Try to send e-mail. If it fails, set watchdog entry.
    if (!$mailer->Send()) {
      // @FIXME
// Could not extract the default value because it is either indeterminate, or
// not scalar. You'll need to provide a default value in
// config/install/mailjet.settings.yml and config/schema/mailjet.schema.yml.
if (\Drupal::config('mailjet.settings')->get('mailjet_debug')) {
          \Drupal::logger('mailjet')->error('Error sending e-mail from @from to @to : @error_message', array(
                     '@from' => $from,
                     '@to' => $to,
                     '@error_message' => $mailer->ErrorInfo));
      }
      return FALSE;
    }

    $mailer->SmtpClose();
    return TRUE;
  }

  /**
   * Splits the input into parts based on the given boundary.
   *
   * Swiped from Mail::MimeDecode, with modifications based on Drupal's coding
   * standards and this bug report: http://pear.php.net/bugs/bug.php?id=6495
   *
   * @param string $input
   *   A string containing the body text to parse.
   * @param string $boundary
   *   A string with the boundary string to parse on.
   *
   * @return array
   *   An array containing the resulting mime parts
   */
  protected function boundarySplit($input, $boundary) {
    $parts       = array();
    $bs_possible = \Drupal\Component\Utility\Unicode::substr($boundary, 2, -2);
    $bs_check    = '\"' . $bs_possible . '\"';

    if ($boundary == $bs_check) {
      $boundary = $bs_possible;
    }

    $tmp = explode('--' . $boundary, $input);

    for ($i = 1; $i < count($tmp); $i++) {
      if (trim($tmp[$i])) {
        $parts[] = $tmp[$i];
      }
    }

    return $parts;
  }

  /**
   * Strips the headers from the body part.
   *
   * @param string $input
   *   A string containing the body part to strip.
   *
   * @return string
   *   A string with the stripped body part.
   */
  protected function removeHeaders($input) {
    $part_array = explode("\n", $input);

    if (strpos($part_array[0], 'Content') !== FALSE) {
      if (strpos($part_array[1], 'Content') !== FALSE) {
        if (strpos($part_array[2], 'Content') !== FALSE) {
            array_shift($part_array);
            array_shift($part_array);
            array_shift($part_array);
        }
        else {
          array_shift($part_array);
          array_shift($part_array);
        }
      }
      else {
        array_shift($part_array);
      }
    }

    $output = implode("\n", $part_array);
    return $output;
  }

  /**
   * Returns a string that is contained within another string.
   *
   * Returns the string from within $source that is some where after $target
   * and is between $beginning_character and $ending_character.
   *
   * @param string $source
   *   A string containing the text to look through.
   * @param string $target
   *   A string containing the text in $source to start looking from.
   * @param string $beginning_character
   *   A string containing the character just before the sought after text.
   * @param string $ending_character
   *   A string containing the character just after the sought after text.
   *
   * @return string
   *   A string with the text found between the $beginning_character and the
   *   $ending_character.
   */
  protected function getSubstrings($source, $target, $beginning_character, $ending_character) {
    $search_start     = strpos($source, $target) + 1;
    $first_character  = strpos($source, $beginning_character, $search_start) + 1;
    $second_character = strpos($source, $ending_character, $first_character) + 1;
    $substring        = \Drupal\Component\Utility\Unicode::substr($source, $first_character, $second_character - $first_character);
    $string_length    = \Drupal\Component\Utility\Unicode::strlen($substring) - 1;

    if ($substring[$string_length] == $ending_character) {
      $substring = \Drupal\Component\Utility\Unicode::substr($substring, 0, $string_length);
    }

    return $substring;
  }
}
