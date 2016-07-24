<?php

namespace Drupal\context_layout\Reaction\Layouts\Form;

use Drupal\context\ContextManager;
use Drupal\context\ContextReactionManager;
use Drupal\context\Reaction\Blocks\Form\BlockFormBase;
use Drupal\Core\Extension\ThemeHandlerInterface;
use Drupal\Core\Form\FormBuilderInterface;
use Drupal\context\ContextInterface;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Component\Plugin\PluginManagerInterface;
use Drupal\Core\Plugin\Context\ContextRepositoryInterface;
use Drupal\layout_plugin\Plugin\Layout\LayoutPluginManagerInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Base form for layout block management.
 *
 * @package Drupal\context_layout\Reaction\Layouts\Form
 */
abstract class LayoutBlockFormBase extends BlockFormBase {

  /**
   * Provides an interface for the discovery and instantiation of context layouts.
   *
   * @var \Drupal\layout_plugin\Plugin\Layout\LayoutPluginManagerInterface
   */
  protected $contextLayoutManager;

  /**
   * {@inheritdoc}
   */
  public function __construct(
    PluginManagerInterface $block_manager,
    ContextRepositoryInterface $contextRepository,
    ThemeHandlerInterface $themeHandler,
    FormBuilderInterface $formBuilder,
    ContextReactionManager $contextReactionManager,
    ContextManager $contextManager,
    LayoutPluginManagerInterface $contextLayoutManager
  ) {
    $this->contextLayoutManager = $contextLayoutManager;
    parent::__construct($block_manager, $contextRepository, $themeHandler, $formBuilder, $contextReactionManager, $contextManager);
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    return new static(
      $container->get('plugin.manager.block'),
      $container->get('context.repository'),
      $container->get('theme_handler'),
      $container->get('form_builder'),
      $container->get('plugin.manager.context_reaction'),
      $container->get('context.manager'),
      $container->get('plugin.manager.context_layout')
    );
  }

  /**
   * Form constructor.
   *
   * @param array $form
   *    An associative array containing the structure of the form.
   *
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *    The current state of the form.
   *
   * @param ContextInterface $context
   *    The context the reaction belongs to.
   *
   * @param string|null $reaction_id
   *    The ID of the blocks reaction the block should be added to.
   *
   * @param string|null $block_id
   *    The ID of the block to show a configuration form for.
   *
   * @return array
   *   Build form structure.
   */
  public function buildForm(array $form, FormStateInterface $form_state, ContextInterface $context = NULL, $reaction_id = NULL, $block_id = NULL) {
    // Submit BlockFormBase build form.
    $form = parent::buildForm($form, $form_state, $context, $reaction_id, $block_id);

    // If a theme was defined in the query use this theme for the block
    // otherwise use the default theme.
    $theme = $this->getRequest()->query->get('theme', $this->themeHandler->getDefault());

    $configuration = $this->block->getConfiguration();
    $regions = $this->getThemeRegionOptions($theme);

    // Get layout from either request or configuration.
    if ($this->getRequest()->query->get('layout') || $configuration['layout']) {
      $layout = $configuration['layout'];
      // Layout from request takes precedence.
      if ($this->getRequest()->query->get('layout')) {
        $layout = $this->getRequest()->query->get('layout');
      }
      $regions = $this->contextLayoutManager->filterLayoutRegions(
        $this->getThemeRegionOptions($theme),
        $layout
      );
    }
    else {
      $fallback = TRUE;
      $layout = $this->contextLayoutManager->getDefaultLayout($fallback);
    }

    // Remove regions unavailable to layout.
    $form['region']['#options'] = $regions;

    $form['layout'] = [
      '#type' => 'value',
      '#value' => $layout,
    ];

    return $form;
  }

  /**
   * Form submission handler.
   *
   * @param array $form
   *    An associative array containing the structure of the form.
   *
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *    The current state of the form.
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $configuration = array_merge($this->block->getConfiguration(), [
      'layout' => $form_state->getValue('layout'),
    ]);
    // Set layout value in block configuration.
    $this->block->setConfiguration($configuration);
    // Let BlockFormBase do the rest.
    parent::submitForm($form, $form_state);
  }

}
