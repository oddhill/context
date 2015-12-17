<?php

namespace Drupal\context\Plugin\DisplayVariant;

use Drupal\Core\Render\Element;
use Drupal\context\ContextManager;
use Drupal\block\BlockRepositoryInterface;
use Drupal\Core\Entity\EntityViewBuilderInterface;
use Drupal\block\Plugin\DisplayVariant\BlockPageVariant;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Provides a page display variant that decorates the main content with blocks.
 *
 * To ensure essential information is displayed, each essential part of a page
 * has a corresponding block plugin interface, so that BlockPageVariant can
 * automatically provide a fallback in case no block for each of these
 * interfaces is placed.
 *
 * @see \Drupal\Core\Block\MainContentBlockPluginInterface
 * @see \Drupal\Core\Block\MessagesBlockPluginInterface
 *
 * @PageDisplayVariant(
 *   id = "context_block_page",
 *   admin_label = @Translation("Page with blocks")
 * )
 */
class ContextBlockPageVariant extends BlockPageVariant {

  /**
   * @var ContextManager
   */
  protected $contextManager;

  /**
   * Constructs a new BlockPageVariant.
   *
   * @param array $configuration
   *   A configuration array containing information about the plugin instance.
   *
   * @param string $plugin_id
   *   The plugin ID for the plugin instance.
   *
   * @param mixed $plugin_definition
   *   The plugin implementation definition.
   *
   * @param string[] $block_list_cache_tags
   *   The Block entity type list cache tags.
   * @param \Drupal\block\BlockRepositoryInterface $block_repository
   *   The block repository.
   *
   * @param \Drupal\Core\Entity\EntityViewBuilderInterface $block_view_builder
   *   The block view builder.
   *
   * @param ContextManager $contextManager
   */
  public function __construct(
    array $configuration,
    $plugin_id,
    $plugin_definition,
    array $block_list_cache_tags,
    BlockRepositoryInterface $block_repository,
    EntityViewBuilderInterface $block_view_builder,
    ContextManager $contextManager
  ) {
    parent::__construct(
      $configuration,
      $plugin_id,
      $plugin_definition,
      $block_repository,
      $block_view_builder,
      $block_list_cache_tags
    );
    $this->contextManager = $contextManager;
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container, array $configuration, $plugin_id, $plugin_definition) {
    return new static(
      $configuration,
      $plugin_id,
      $plugin_definition,
      $container->get('entity.manager')->getDefinition('block')->getListCacheTags(),
      $container->get('block.repository'),
      $container->get('entity.manager')->getViewBuilder('block'),
      $container->get('context.manager')
    );
  }

  /**
   * {@inheritdoc}
   */
  public function build() {
    // Let the block page variant build it's render array before executing any
    // block reactions.
    $build = parent::build();

    // Execute each block reaction and let them modify the page build.
    foreach ($this->contextManager->getActiveReactions('blocks') as $reaction) {
      $build = $reaction->execute($build, $this->title, $this->mainContent);
    }

    return $build;
  }

}
