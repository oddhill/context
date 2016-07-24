<?php

namespace Drupal\context_layout\Plugin\ContextReaction;

use Drupal\context\ContextReactionManager;
use Drupal\Core\Link;
use Drupal\Core\Url;
use Drupal\context\ContextInterface;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Theme\ThemeManagerInterface;
use Drupal\context\ContextReactionPluginBase;
use Drupal\Core\Extension\ThemeHandlerInterface;
use Drupal\Core\Plugin\ContainerFactoryPluginInterface;
use Drupal\Core\Plugin\Context\ContextHandlerInterface;
use Drupal\Core\Plugin\Context\ContextRepositoryInterface;
use Drupal\layout_plugin\Plugin\Layout\LayoutPluginManagerInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Provides a content reaction that will let you place blocks in the current
 * layout's regions.
 *
 * @ContextReaction(
 *   id = "layouts",
 *   label = @Translation("Layouts")
 * )
 */
class Layouts extends ContextReactionPluginBase implements ContainerFactoryPluginInterface {

  /**
   * Manages the list of available themes.
   *
   * @var \Drupal\Core\Extension\ThemeHandlerInterface
   */
  protected $themeHandler;

  /**
   * Offers a global context repository.
   *
   * @var \Drupal\Core\Plugin\Context\ContextRepositoryInterface
   */
  protected $contextRepository;

  /**
   * Provides an interface for handling sets of contexts.
   *
   * @var \Drupal\Core\Plugin\Context\ContextHandlerInterface
   */
  protected $contextHandler;

  /**
   * Provides a high level access to the active theme and methods to use it.
   *
   * @var \Drupal\Core\Theme\ThemeManagerInterface
   */
  protected $themeManager;

  /**
   * Provides an interface for the discovery and instantiation of layout plugins.
   *
   * @var \Drupal\layout_plugin\Plugin\Layout\LayoutPluginManagerInterface
   */
  protected $contextLayoutManager;

  /**
   * Base class for context plugin manager.
   *
   * @var \Drupal\context\ContextReactionManager
   */
  protected $contextReactionManager;

  /**
   * Layout ID (machine name).
   *
   * @var string|null
   */
  protected $layout = NULL;

  /**
   * Drupal\context\Plugin\ContextReaction\Blocks instance.
   *
   * @var object
   */
  protected $blocks;

  /**
   * {@inheritdoc}
   */
  public function __construct(
    array $configuration,
    $pluginId,
    $pluginDefinition,
    ThemeHandlerInterface $themeHandler,
    ThemeManagerInterface $themeManager,
    ContextRepositoryInterface $contextRepository,
    ContextHandlerInterface $contextHandler,
    LayoutPluginManagerInterface $contextLayoutManager,
    ContextReactionManager $contextReactionManager
  ) {
    $this->themeHandler = $themeHandler;
    $this->contextRepository = $contextRepository;
    $this->contextHandler = $contextHandler;
    $this->themeManager = $themeManager;
    $this->contextLayoutManager = $contextLayoutManager;
    $this->contextReactionManager = $contextReactionManager;
    // Create a Drupal\context\Plugin\ContextReaction\Blocks instance.
    $this->blocks = $contextReactionManager->createInstance('blocks');
    parent::__construct($configuration, $pluginId, $pluginDefinition);
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container, array $configuration, $pluginId, $pluginDefinition) {
    return new static(
      $configuration,
      $pluginId,
      $pluginDefinition,
      $container->get('theme_handler'),
      $container->get('theme.manager'),
      $container->get('context.repository'),
      $container->get('context.handler'),
      $container->get('plugin.manager.context_layout'),
      $container->get('plugin.manager.context_reaction')
    );
  }

  /**
   * Executes the plugin.
   *
   * @param array $build
   *   The current build of the page.
   *
   * @param string|null $title
   *   The page title.
   *
   * @param string|null $main_content
   *   The main page content.
   *
   * @return array
   */
  public function execute(array $build = array(), $title = NULL, $main_content = NULL) {
    // Check for an existing context layout from page build.
    if (isset($build['#layout'])) {
      $layout = $build['#layout']['id'];
      $layout_regions = $this->contextLayoutManager
        ->loadLayout($build['#layout']['id'])
        ->getRegionDefinitions();
    }
    else {
      $layout = $this->layout;
      $layout_regions = $this->contextLayoutManager
        ->loadLayout($this->layout)
        ->getRegionDefinitions($this->layout);
    }
    $layout_regions = array_keys($layout_regions);

    // Use the currently active theme to fetch blocks.
    $theme = $this->themeManager->getActiveTheme()->getName();
    $theme_regions = array_keys(system_region_list($theme));

    // Get Blocks context reaction build array.
    $build = $this->blocks->execute($build, $title, $main_content);

    // Remove blocks that are not available in layout's regions.
    foreach ($build as $key => $value) {
      $in_theme = in_array($key, $theme_regions);
      $in_layout = in_array($key, $layout_regions);
      if ($in_theme && !$in_layout) {
        unset($build[$key]);
      }
    }

    /** @var $layout \Drupal\layout_plugin\Layout */
    $layout = $this->contextLayoutManager->loadLayout($layout);
    return $layout->build($build);
  }

  /**
   * {@inheritdoc}
   */
  public function summary() {
    return $this->t('Lets you select a layout and blocks.');
  }

  /**
   * {@inheritdoc}
   */
  public function buildConfigurationForm(array $form, FormStateInterface $form_state, ContextInterface $context = NULL) {
    // Check for available layouts and display appropriate form.
    $options = $this->getLayoutOptions();
    if (!$options) {
      return $this->invalidConfigurationForm($form);
    }
    else {
      return $this->validConfigurationForm($form, $form_state, $context);
    }
  }

  /**
   * Display invalid build form.
   *
   * @param array $form
   *   An associative array containing the structure of the form.
   *
   * @return array
   *   Modified $form.
   */
  private function invalidConfigurationForm(array $form) {
    $form['no_layouts'] = [
      '#type' => 'item',
      '#title' => t('No Defined Layouts'),
      '#description' => t('You have no defined \'full\' type layouts. You
      must register layouts with the type of \'full\' within your theme. ') .
        Link::fromTextAndUrl(
          'See Layout Plugin documentation',
          Url::fromUri('https://www.drupal.org/node/2578731'))->toString()
    ];
    return $form;
  }

  /**
   * Display valid configuration build form.
   *
   * @param array $form
   *    An associative array containing the structure of the form.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *    The current state of the form.
   * @param \Drupal\context\ContextInterface|null $context
   *    Current context.
   *
   * @return array
   *   Modified $form.
   */
  private function validConfigurationForm(array $form, FormStateInterface $form_state, ContextInterface $context = NULL) {
    $layout = $form_state->getValue('layout', $this->getLayout());

    // The layout to use for the context reaction.
    $form['layout'] = [
      '#type' => 'select',
      '#title' => t('Layout'),
      '#options' => $this->getLayoutOptions([
        'group_by_category' => TRUE,
        'default' => TRUE,
      ]),
      '#default_value' => $layout,
      '#ajax' => [
        'url' => Url::fromRoute('context.reaction.layouts.layout_select', [
          'context' => $context->id(),
        ]),
      ],
    ];

    // Build blocks reaction build configuration form.
    $form += $this->blocks->buildConfigurationForm($form, $form_state, $context);

    // Filter the blocks reaction build configuration form.
    $this->filterForm($form, $form_state, $context);

    return $form;
  }

  /**
   * Get layout regions from collection of blocks.
   *
   * @param string $theme
   *   The theme to get blocks for.
   * @param string $layout
   *    Layout ID (machine name).
   *
   * @return array
   *    Region ID's.
   */
  private function getUsedLayoutRegions($theme, $layout) {
    $layout_regions  = [];
    $regions = $this->blocks->getBlocks()->getAllByRegion($theme);
    foreach ($regions as $region_id => $region) {
      foreach ($region as $theme_block) {
        if ($layout == $theme_block->configuration['layout']) {
          $layout_regions[] = $region_id;
        }
      }
    }
    return $layout_regions;
  }

  /**
   * Filter blocks reaction build configuration form.
   *
   * @param array $form
   *    An associative array containing the structure of the form.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *    The current state of the form.
   * @param \Drupal\context\ContextInterface|null $context
   *    Current context.
   *
   * @return array
   *   Modified $form.
   */
  private function filterForm(array &$form, FormStateInterface &$form_state, ContextInterface $context) {
    $layout = $form_state->getValue('layout', $this->getLayout());
    $default_theme = $this->themeHandler->getDefault();
    $theme = $form_state->getValue('theme', $default_theme);
    $system_regions = system_region_list($theme);

    /** @var \Drupal\Core\Block\BlockPluginInterface $blocks */
    $blocks = $this->blocks->getBlocks()->getAllByRegion($theme);

    // Get layout region ID's.
    $layout_regions = array_keys(
      $this->contextLayoutManager->loadLayout($layout)
        ->getRegionDefinitions()
    );

    $used_layout_regions = $this->getUsedLayoutRegions($theme, $layout);

    // Filter theme selection route.
    $form['theme']['#ajax']['url'] = Url::fromRoute(
      'context.reaction.layouts.theme_select', [
        'context' => $context->id(),
      ]
    );

    // Filter block library route.
    $form['blocks']['block_add']['#url'] = Url::fromRoute(
      'context.reaction.layouts.library', [
        'context' => $context->id(),
        'reaction_id' => $this->getPluginId(),
      ], [
        'query' => [
          'theme' => $theme,
          'layout' => $layout,
        ],
      ]
    );

    // Tabledrag row count.
    $table_count = 0;
    foreach ($system_regions as $region => $title) {

      // Remove regions unavailable to layout.
      $region_id = 'region-' . $region;
      $region_message_id = $region_id . '-message';
      $layout_has_region = in_array($region, $layout_regions);
      $layout_used_region = in_array($region, $used_layout_regions);

      if (!$layout_has_region) {
        unset($form['blocks']['blocks'][$region_message_id]);
        unset($form['blocks']['blocks'][$region_id]);
      }

      // Apply empty classes to tabledrag rows that may now be empty.
      if ($layout_has_region && !$layout_used_region) {
        if (isset($form['blocks']['blocks'][$region_message_id])) {
          foreach ($form['blocks']['blocks'][$region_message_id]['#attributes']['class'] as $i => $class) {
            if ('region-populated' == $class) {
              unset($form['blocks']['blocks'][$region_message_id]['#attributes']['class'][$i]);
              $form['blocks']['blocks'][$region_message_id]['#attributes']['class'][] = 'region-empty';
            }
          }
        }
      }

      // Remove tabledrag rows unavailable to layout.
      if (!in_array($region, $layout_regions)) {
        unset($form['blocks']['blocks']['#tabledrag'][$table_count]);
        unset($form['blocks']['blocks']['#tabledrag'][$table_count + 1]);
      }

      // Tabledrag has 2 rows per region.
      $table_count += 2;

      if (isset($blocks[$region])) {

        // Loop through blocks within the current region.
        foreach ($blocks[$region] as $block_id => $block) {

          // Route args for block's operations.
          $url_args = [
            'context' => $context->id(),
            'reaction_id' => $this->getPluginId(),
            'block_id' => $block_id,
          ];

          // Filter block edit route.
          $form['blocks']['blocks'][$block_id]['operations']['#links']['edit']['url'] = Url::fromRoute(
            'context.reaction.layouts.block_edit',
            $url_args
          );

          // Filter block delete route.
          $form['blocks']['blocks'][$block_id]['operations']['#links']['delete']['url'] = Url::fromRoute(
            'context.reaction.layouts.block_delete',
            $url_args
          );

          // Remove blocks that are in a region unavailable to layout.
          if (!in_array($region, $layout_regions)) {
            unset($form['blocks']['blocks'][$block_id]);
          }
        }
      }
    }

    // Remove block instances that are not in the current layout.
    foreach ($blocks as $region_id => $region_blocks) {
      foreach ($region_blocks as $block_id => $block) {
        if ($layout != $block->configuration['layout']) {
          unset($form['blocks']['blocks'][$block_id]);
        }
      }
    }
  }

  /**
   * {@inheritdoc}
   */
  public function defaultConfiguration() {
    $configuration = parent::defaultConfiguration();
    $configuration += $this->blocks->defaultConfiguration();
    $configuration['layout'] = NULL;
    return $configuration;
  }

  /**
   * {@inheritdoc}
   */
  public function setConfiguration(array $configuration) {
    $this->blocks->setConfiguration($configuration);
    $this->configuration = $configuration + $this->defaultConfiguration();
    if (isset($configuration['layout'])) {
      $this->layout = $configuration['layout'];
    }
    return $this;
  }

  /**
   * {@inheritdoc}
   */
  public function getConfiguration() {
    $configuration = $this->blocks->getConfiguration();
    // Restore ID to configuration.
    $configuration['id'] = 'layouts';
    $configuration['layout'] = $this->configuration['layout'];
    $configuration += parent::getConfiguration();
    return $configuration;
  }

  /**
   * {@inheritdoc}
   */
  public function submitConfigurationForm(array &$form, FormStateInterface $form_state) {
    $this->blocks->submitConfigurationForm($form, $form_state);
    $this->configuration['layout'] = $form_state->getValue('layout');
  }

  /**
   * Get most appropriate layout.
   *
   * @return string
   *    Layout ID (machine name).
   */
  protected function getLayout() {
    // Current layout.
    if ($this->layout) {
      return $this->layout;
    }
    // Default layout.
    elseif ($this->contextLayoutManager->getDefaultLayout()) {
      return 'default';
    }
    // Fallback layout.
    else {
      $fallback = TRUE;
      return $this->contextLayoutManager->getDefaultLayout($fallback);
    }
  }

  /**
   * Return formatted layout region options.
   *
   * @param array $params
   *   (optional) An associative array with the following keys:
   *   - group_by_category: (bool) If set to TRUE, return an array of arrays
   *   grouped by the category name; otherwise, return a single-level
   *   associative array.
   *
   * @return array
   *   Layout options, as array.
   */
  protected function getLayoutOptions(array $params = []) {
    $default = !empty($params['default']);
    $layouts = $this->contextLayoutManager->getLayoutOptions($params);
    // Prepend default layout option.
    if ($default && $this->contextLayoutManager->getDefaultLayout()) {
      $default_layout = $this->contextLayoutManager->createInstance(
        $this->contextLayoutManager->getDefaultLayout()
      );
      $label = $default_layout->getPluginDefinition()['label'] . ' (' . t('Default') . ')';
      $layouts = ['default' => $label] + $layouts;
    }
    return $layouts;
  }

  /**
   * Add a new block.
   *
   * @param array $configuration
   *    Reaction configuration.
   *
   * @return string
   *    Generated UUID.
   */
  public function addBlock(array $configuration) {
    return $this->blocks->addBlock($configuration);
  }

  /**
   * Update an existing blocks configuration.
   *
   * @param string $blockId
   *    The ID of the block to update.
   *
   * @param $configuration
   *    The updated configuration for the block.
   *
   * @return object
   *    $this.
   */
  public function updateBlock($blockId, array $configuration) {
    return $this->blocks->updateBlock($blockId, $configuration);
  }

  /**
   * Remove a block from block instances.
   *
   * @param string $blockId
   *    The ID of the block to get.
   *
   * @return object
   *    $this.
   */
  public function removeBlock($blockId) {
    return $this->blocks->removeBlock($blockId);
  }

  /**
   * Get a block by id.
   *
   * @param string $blockId
   *    The ID of the block to get.
   *
   * @return object
   *    BlockPluginInterface instance.
   */
  public function getBlock($blockId) {
    return $this->blocks->getBlock($blockId);
  }

}
