<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Form\CronJobEnableForm.
 */

namespace Drupal\ultimate_cron\Form;

use Drupal\Core\Entity\EntityConfirmFormBase;
use Drupal\Core\Form\FormStateInterface;


class CronJobEnableForm extends EntityConfirmFormBase {

  /**
   * {@inheritdoc}
   */
  public function getQuestion() {
    // @todo update for enabling
    return $this->t('Do you really want to disable cron job @cronjob_id?', array(
      '@cronjob_id' => $this->getEntity()->label(),
    ));
  }

  /**
   * {@inheritdoc}
   */
  public function getCancelUrl() {
    return $this->getEntity()->urlInfo();
  }

  /**
   * {@inheritdoc}
   */
  public function getConfirmText() {
    return $this->t('Enable');
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $this->entity->enable()->save();
    drupal_set_message($this->t('Enabled cron job %cronjob.', array('%cronjob' => $this->entity->label())));

    $form_state->setRedirectUrl($this->getCancelUrl());
  }
  
}
