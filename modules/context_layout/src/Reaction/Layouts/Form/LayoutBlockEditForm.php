<?php

namespace Drupal\context_layout\Reaction\Layouts\Form;

/**
 * Manage layout block edit form.
 */
class LayoutBlockEditForm extends LayoutBlockFormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'context_reaction_layouts_edit_block_form';
  }

  /**
   * {@inheritdoc}
   */
  protected function getSubmitValue() {
    return $this->t('Update block');
  }

  /**
   * {@inheritdoc}
   */
  protected function prepareBlock($block_id) {
    return $this->reaction->getBlock($block_id);
  }

}
