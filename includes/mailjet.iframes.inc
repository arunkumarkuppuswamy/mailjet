<?php

/**
 * Registration iframe form.
 */
function mailjet_register_iframe() {
    global $base_url;
    $language = \Drupal::languageManager()->getCurrentLanguage();
  
    $langCodesMap = array('en' => 'en_US', 'fr' => 'fr_FR', 'de' => 'de_DE', 'es' => 'es_ES');
    $defaultLang = 'en';
    $locale = isset($langCodesMap[$language->language]) ? $langCodesMap[$language->language] : $langCodesMap[$defaultLang];
    
    $default_url = IFRAME_URL."reseller/signup?r=commerceguys&sp=display&p=drupal-3.0&locale={$locale}&cb=";
    $callbackurl = urlencode($base_url . '?q=/admin/config/system/mailjet/api/register/alter_callback');
    return '<iframe src="' . $default_url . $callbackurl . '" width="100%" height="1080px" frameborder="0"></iframe>';
}

/**
 * Check for user api.
 */
function mailjet_plans_iframe() {
  // @FIXME
// Could not extract the default value because it is either indeterminate, or
// not scalar. You'll need to provide a default value in
// config/install/mailjet.settings.yml and config/schema/mailjet.schema.yml.
// @FIXME
// Could not extract the default value because it is either indeterminate, or
// not scalar. You'll need to provide a default value in
// config/install/mailjet.settings.yml and config/schema/mailjet.schema.yml.
if (!\Drupal::config('mailjet.settings')->get('mailjet_username') && !\Drupal::config('mailjet.settings')->get('mailjet_password')) {
    drupal_set_message(t('You need to add your Mailjet API details before you can continue'), 'warning');
    drupal_goto('admin/config/system/mailjet/api');
  }

  // @FIXME
// // @FIXME
// // This looks like another module's variable. You'll need to rewrite this call
// // to ensure that it uses the correct configuration object.
// $token = variable_get("APItoken");

  if (!empty($token)) {
      
    $language = \Drupal::languageManager()->getCurrentLanguage();
  
    $langCodesMap = array('en' => 'en_US', 'fr' => 'fr_FR', 'de' => 'de_DE', 'es' => 'es_ES');
    $defaultLang = 'en';
    $locale = isset($langCodesMap[$language->language]) ? $langCodesMap[$language->language] : $langCodesMap[$defaultLang];

    $callbackurl = 'r=commerceguys&t='.$token.'&show_menu=none&sp=display&locale='.$locale;

    $default_url = IFRAME_URL."reseller/pricing/?";

    return '<iframe src="' . $default_url . $callbackurl . '" width="100%" height="1080px" frameborder="0"></iframe>';
  }
  return t('Unable to generate upgrade form.');
}
