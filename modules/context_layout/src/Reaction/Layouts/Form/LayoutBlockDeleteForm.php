<?php

namespace Drupal\context_layout\Reaction\Layouts\Form;

use Drupal\context\Reaction\Blocks\Form\BlockDeleteForm;
use Drupal\context\ContextInterface;
use Drupal\Core\Form\ConfirmFormBase;
use Drupal\Core\Form\FormStateInterface;

/**
 * Manage layout block delete form.
 *
 * @package Drupal\context_layout\Reaction\Layouts\Form
 */
class LayoutBlockDeleteForm extends BlockDeleteForm {

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'context_reaction_layouts_delete_block_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, ContextInterface $context = NULL, $block_id = NULL) {
    $this->context = $context;
    $this->reaction = $this->context->getReaction('layouts');
    $this->block = $this->reaction->getBlock($block_id);

    // Build Drupal\Core\Form\ConfirmFormBase form.
    $form = ConfirmFormBase::buildForm($form, $form_state);

    // Remove the cancel button if this is an AJAX request since Drupals built
    // in modal dialogues does not handle buttons that are not a primary
    // button very well.
    if ($this->getRequest()->isXmlHttpRequest()) {
      unset($form['actions']['cancel']);
    }

    // Submit the form with AJAX if possible.
    $form['actions']['submit']['#ajax'] = [
      'callback' => '::submitFormAjax',
    ];

    return $form;
  }

}
