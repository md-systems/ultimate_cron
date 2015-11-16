<?php
/**
 * @file
 * Contains \Drupal\ultimate_cron\Form\CronDisableForm.
 */

namespace Drupal\ultimate_cron\Form;

use Drupal\Core\Entity\ContentEntityConfirmFormBase;
use Drupal\Core\Entity\EntityManagerInterface;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\StringTranslation\TranslationInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;


class CronDisableForm extends ContentEntityConfirmFormBase {

  /**
   * Constructs a new instance.
   *
   * @param \Drupal\Core\Entity\EntityManagerInterface $entity_manager
   *   The entity manager.
   */
  public function __construct(EntityManagerInterface $entity_manager) {
    parent::__construct($entity_manager);
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    return new static($container->get('entity.manager'));
  }

  /**
   * {@inheritdoc}
   */
  public function getQuestion() {
    // @todo update for enabling
    return $this->t('Do you really want to disable cron job #@cronjob_id?', array(
      '@cronjob_id' => $this->getEntity()->id(),
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
    return $this->t('Disable');
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $this->entity->disable()->save();
    drupal_set_message($this->t('Disabled text format %format.', array('%format' => $this->entity->label())));

    $form_state->setRedirectUrl($this->getCancelUrl());
  }
  
}
