<?php

namespace Drupal\context_layout\Reaction\Layouts\Form;

/**
 * Manage layout block add form.
 */
class LayoutBlockAddForm extends LayoutBlockFormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'context_reaction_layouts_add_block_form';
  }

  /**
   * {@inheritdoc}
   */
  protected function getSubmitValue() {
    return $this->t('Add block');
  }

  /**
   * {@inheritdoc}
   */
  protected function prepareBlock($block_id) {
    return $this->blockManager->createInstance($block_id);
  }

}
