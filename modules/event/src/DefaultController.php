<?php
namespace Drupal\mailjet_event;

/**
 * Default controller for the mailjet_event module.
 */
class DefaultController extends ControllerBase {

  public function mailjet_event_callback() {
    // No Event sent.
    if ($_SERVER['REQUEST_METHOD'] != 'POST') {
      drupal_not_found();
    }

    $post = trim(file_get_contents('php://input'));

    // Decode Trigger Informations.
    $event = json_decode($post, TRUE);

    // No Informations sent with the Event.
    if (!is_array($event) || !isset($event['event'])) {
      drupal_not_found();
    }

    if (isset($event['email'])) {
      $mail_user = user_load_by_mail($event['email']);
    }
    elseif (isset($event['original_address'])) {
      $mail_user = user_load_by_mail($event['original_address']);
    }

    $mailjet_event = mailjet_event_new($mail_user, $event);
    mailjet_event_save($mailjet_event);

    switch ($event['event']) {
      case 'open' :
        rules_invoke_event('mailjet_open', $mailjet_event);

        break;
      case 'click' :
        rules_invoke_event('mailjet_click', $mailjet_event);
        break;

      case 'bounce' :
        rules_invoke_event('mailjet_bounce', $mailjet_event);
        break;

      case 'spam' :
        rules_invoke_event('mailjet_spam', $mailjet_event);
        break;

      case 'blocked' :
        rules_invoke_event('mailjet_blocked', $mailjet_event);
        break;

      case 'unsub' :
        rules_invoke_event('mailjet_unsub', $mailjet_event);
        break;

      // No handler
      default :
        drupal_not_found();
        break;
    }

    drupal_send_headers();
    drupal_exit();
  }

}
