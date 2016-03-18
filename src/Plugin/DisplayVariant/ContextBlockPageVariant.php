<?php

namespace Drupal\context\Plugin\DisplayVariant;

use Drupal\block\Plugin\DisplayVariant\BlockPageVariant;
use Drupal\context\ContextManager;
use Drupal\Core\Display\PageVariantInterface;
use Drupal\Core\Plugin\ContainerFactoryPluginInterface;

/**
 * Provides a page display variant that decorates the main content with blocks.
 *
 * @see \Drupal\Core\Block\MainContentBlockPluginInterface
 * @see \Drupal\Core\Block\MessagesBlockPluginInterface
 *
 * @PageDisplayVariant(
 *   id = "context_block_page",
 *   admin_label = @Translation("Page with blocks")
 * )
 */
class ContextBlockPageVariant extends BlockPageVariant implements PageVariantInterface, ContainerFactoryPluginInterface {

  /**
   * @var ContextManager
   */
  protected $contextManager;

  /**
   * Get the context manager
   *
   * @return \Drupal\context\ContextManager|mixed
   */
  private function getContextManager() {
    if (!isset($this->contextManager)) {
      $this->contextManager =  \Drupal::service('context.manager');
    }
    return $this->contextManager;
  }
  
  /**
   * {@inheritdoc}
   */
  public function build() {
    $build = parent::build();

    $reactionblocks = $this->getContextManager()->getActiveReactions('blocks');

    // Execute each block reaction and let them modify the page build.
    foreach ($reactionblocks as $reaction) {
      $build = $reaction->execute($build, $this->title, $this->mainContent);
    }

    return $build;
  }

}
